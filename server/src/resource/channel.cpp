#include "resource/channel.h"
#include "resource/utils.h"
#include "resource/role_utils.h"
#include "resource/json_utils.h"

server_channel_resource::server_channel_resource(webserver& ws, db_connection_pool& pool, const config& cfg,
					socket_main_server& sserv, socket_vc_server& vcserv):
	base_resource(ws, "/servers/{server_id}/channels", pool, cfg),
	sserv{sserv}, vcserv{vcserv}
{
	set_allowing("GET", true);
	set_allowing("POST", true);
}

std::shared_ptr<http_response> server_channel_resource::render_GET(const http_request& req)
{
	base_resource::render_GET(req);

	int user_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_server_id(req, tx, user_id, server_id);
	if(err) return err;

	std::string wl_check;
	auto no_ch_manage_perms = role_utils::check_permission(req, tx, server_id, user_id,
								role_utils::perms1, PERM1_MANAGE_CHANNELS);
	if(no_ch_manage_perms)
	       	wl_check = " AND ((array_length(wl_users, 1) IS NULL AND array_length(wl_roles, 1) IS NULL) OR array_position(wl_users, $2) IS NOT NULL OR EXISTS(SELECT role_id FROM user_x_server WHERE user_id = $2 AND array_position(wl_roles, role_id) IS NOT NULL))";

	nlohmann::json res = nlohmann::json::array();
	pqxx::result r = tx.exec("SELECT channels.channel_id, name, type, notification_count, wl_users, wl_roles FROM channels "
				 "LEFT JOIN notifications ON notifications.channel_id = channels.channel_id "
				 "AND notifications.user_id = $2 "
				 "WHERE channels.server_id = $1" + wl_check + " ORDER BY channels.channel_id"
				, pqxx::params(server_id, user_id));

	for(size_t i = 0; i < r.size(); ++i){
		pqxx::result role_rows = tx.exec("SELECT role_id, perms1 FROM channel_x_role "
						 "WHERE channel_id = $1",
						 pqxx::params(r[i]["channel_id"].as<int>()));
		nlohmann::json channel_json = json_utils::channel_from_row(r[i], &role_rows, true);
		if(channel_json["type"] == CHANNEL_VOICE)
			channel_json["vc_users"] = vcserv.get_channel_users(channel_json["id"], tx, user_id);
		res += channel_json;
	}

	return create_response::string(req, res.dump(), 200);
}
std::shared_ptr<http_response> server_channel_resource::render_POST(const http_request& req)
{
	base_resource::render_POST(req);

	int user_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_server_id(req, tx, user_id, server_id);
	if(err) return err;

	err = role_utils::check_permission(req, tx, server_id, user_id,
						role_utils::perms1, PERM1_MANAGE_CHANNELS);
	if(err) return err;

	// Parse JSON arguments
	nlohmann::json body;
	err = json_utils::from_content(req, body);
	if(err)
		return err;

	JSON_GET_STRING(body, name);
	JSON_GET_UNSIGNED(body, type);
	if(type != CHANNEL_TEXT && type != CHANNEL_VOICE)
		return create_response::string(req, "Unknown channel type", 400);

	std::vector<int> wl_users, wl_roles;
	if(body["wl_users"].is_array()){
		err = json_utils::get_array(req, body, nlohmann::json::value_t::number_unsigned, "wl_users", wl_users);
		if(err)
			return err;
		wl_users = resource_utils::get_valid_user_ids_vector(wl_users, tx, server_id);
	}
	if(body["wl_roles"].is_array()){
		err = json_utils::get_array(req, body, nlohmann::json::value_t::number_unsigned, "wl_roles", wl_roles);
		if(err)
			return err;
		wl_roles = resource_utils::get_valid_role_ids_vector(wl_roles, tx, server_id);
	}

	// Check channel count and insert
	pqxx::result r = tx.exec("SELECT channel_id FROM channels WHERE server_id = $1", pqxx::params(server_id));
	if(r.size() >= cfg.max_channels_per_server)
		return create_response::string(req, "Server has more than " + std::to_string(cfg.max_channels_per_server) + " channels", 403);

	int channel_id;
	socket_event ev;
	try{
		std::string wl_columns, wl_params;
		pqxx::params pr(server_id, name, type);
		if(wl_users.size()){
			pr.append(wl_users);
			wl_columns += ", wl_users";
			wl_params += ", $" + std::to_string(pr.size());
		}
		if(wl_roles.size()){
			pr.append(wl_roles);
			wl_columns += ", wl_roles";
			wl_params += ", $" + std::to_string(pr.size());
		}

		r = tx.exec("INSERT INTO channels(server_id, name, type" + wl_columns + ") VALUES($1, $2, $3" + wl_params +
			    ") RETURNING channel_id, name, type, wl_users, wl_roles",
			    pr);
		channel_id = r[0]["channel_id"].as<int>();
		ev.data = json_utils::channel_from_row(r[0]);
	} catch(pqxx::data_exception& e){
		return create_response::string(req, "Channel name is too long", 400);
	}
	tx.commit();

	json_utils::set_ids(ev.data, server_id);
	ev.name = "channel_created";
	sserv.send_to_server(server_id, tx, ev, -1,
				wl_users, wl_roles);

	return create_response::string(req, std::to_string(channel_id), 200);
}

channel_resource::channel_resource(webserver& ws, db_connection_pool& pool, const config& cfg,
				socket_main_server& sserv, socket_vc_server& vcserv):
	base_resource(ws, "/channels/{channel_id}", pool, cfg),
	sserv{sserv}, vcserv{vcserv}
{
	set_allowing("GET", true);
	set_allowing("PUT", true);
	set_allowing("DELETE", true);
}
std::shared_ptr<http_response> channel_resource::render_GET(const http_request& req)
{
	base_resource::render_GET(req);

	int user_id, channel_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_channel_id(req, tx, user_id, server_id, channel_id);
	if(err) return err;

	pqxx::result r = tx.exec("SELECT channels.channel_id, name, type, user1_id, user2_id, notification_count, wl_users, wl_roles FROM channels "
				 "LEFT JOIN notifications ON notifications.channel_id = channels.channel_id "
				 "AND notifications.user_id = $2 "
				 "WHERE channels.channel_id = $1"
				 , pqxx::params(channel_id, user_id));
	pqxx::result role_rows = tx.exec("SELECT role_id, perms1 FROM channel_x_role "
					 "WHERE channel_id = $1",
					 pqxx::params(channel_id));
	nlohmann::json channel_json = json_utils::channel_from_row(r[0], &role_rows, true, user_id);

	if(channel_json["type"] == CHANNEL_VOICE)
		channel_json["vc_users"] = vcserv.get_channel_users(channel_id, tx, user_id);

	return create_response::string(req, channel_json.dump(), 200);
}

void get_old_wl_users(const std::vector<int>& wl_users, const std::vector<int>& wl_roles,
			int server_id, pqxx::work& tx, std::vector<int>& out)
{
	std::string where_user, where_role;
	if(wl_users.size())
		where_user = "user_id IN (" + resource_utils::int_array_to_string(wl_users) + ") ";
	if(wl_roles.size())
		where_role = std::string(where_user.size() ? " OR " : "") + "role_id IN (" + resource_utils::int_array_to_string(wl_roles) + ") ";
	pqxx::result r = tx.exec("SELECT user_id FROM user_x_server NATURAL JOIN servers WHERE server_id = $1 " +
				 (where_user.size() || where_role.size() ? "AND (owner_id = user_id OR (" + where_user + where_role + "))" : "") +
				 " GROUP BY user_id",
				 pqxx::params(server_id));

	out.reserve(r.size());
	for(auto i = r.begin(); i != r.end(); ++i)
		out.push_back((*i)["user_id"].as<int>());
}
void get_new_wl_users(const std::vector<int>& wl_users, const std::vector<int>& wl_roles,
			const std::vector<int>* new_wl_users, const std::vector<int>* new_wl_roles, 
			int server_id, pqxx::work& tx, std::vector<int>& out)
{
	std::string where_user, where_role;
	if(new_wl_users ? new_wl_users->size() : wl_users.size())
		where_user = "user_id IN (" + resource_utils::int_array_to_string(new_wl_users ? *new_wl_users : wl_users) + ") ";
	if(new_wl_roles ? new_wl_roles->size() : wl_roles.size())		
		where_role = std::string(where_user.size() ? " OR " : "") + "role_id IN (" + resource_utils::int_array_to_string(new_wl_roles ? *new_wl_roles : wl_roles) + ") ";
	pqxx::result r = tx.exec("SELECT user_id FROM user_x_server NATURAL JOIN servers WHERE server_id = $1 " +
				 (where_user.size() || where_role.size() ? "AND (owner_id = user_id OR (" + where_user + where_role + "))" : "") +
				 " GROUP BY user_id",
				 pqxx::params(server_id));

	out.reserve(r.size());
	for(auto i = r.begin(); i != r.end(); ++i)
		out.push_back((*i)["user_id"].as<int>());
}

std::shared_ptr<http_response> channel_resource::render_PUT(const http_request& req)
{
	base_resource::render_PUT(req);

	int user_id, channel_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_channel_id(req, tx, user_id, server_id, channel_id);
	if(err) return err;

	if(server_id == -1)
		return create_response::string(req, "Cannot change a DM channel", 403);

	err = role_utils::check_permission(req, tx, server_id, user_id,
						role_utils::perms1, PERM1_MANAGE_CHANNELS);
	if(err) return err;

	nlohmann::json body;
	err = json_utils::from_content(req, body);
	if(err)
		return err;

	std::unique_ptr<std::vector<int>> new_wl_users, new_wl_roles;
	pqxx::result ch_row = tx.exec("SELECT * FROM channels WHERE channel_id = $1", pqxx::params(channel_id));

	bool updated = false;
	if(body["name"].type() == nlohmann::json::value_t::string){
		std::string name = body["name"].get<std::string>();
		try{
			tx.exec("UPDATE channels SET name = $1 WHERE channel_id = $2", pqxx::params(name, channel_id));
		} catch(pqxx::data_exception& e){
			return create_response::string(req, "Channel name is too long", 400);
		}
		updated = true;
	}
	if(body["type"].type() == nlohmann::json::value_t::number_unsigned){
		int type = body["type"].get<int>();
		if(type != CHANNEL_TEXT && type != CHANNEL_VOICE)
			return create_response::string(req, "Unknown channel type", 400);
		tx.exec("UPDATE channels SET type = $1 WHERE channel_id = $2", pqxx::params(type, channel_id));
		updated = true;
	}
	if(body["wl_users"].is_array()){
		new_wl_users = std::make_unique<std::vector<int>>();
		err = json_utils::get_array(req, body, nlohmann::json::value_t::number_unsigned, "wl_users", *new_wl_users);
		if(err)
			return err;
		if(new_wl_users->size() > cfg.max_wl_users_per_channel)
			return create_response::string(req, "Too many whitelisted users per channel", 400);
		*new_wl_users = resource_utils::get_valid_user_ids_vector(*new_wl_users, tx, server_id);
		tx.exec("UPDATE channels SET wl_users = $1 WHERE channel_id = $2", pqxx::params(*new_wl_users, channel_id));
		updated = true;
	}
	if(body["wl_roles"].is_array()){
		new_wl_roles = std::make_unique<std::vector<int>>();
		err = json_utils::get_array(req, body, nlohmann::json::value_t::number_unsigned, "wl_roles", *new_wl_roles);
		if(err)
			return err;
		*new_wl_roles = resource_utils::get_valid_role_ids_vector(*new_wl_roles, tx, server_id);
		tx.exec("UPDATE channels SET wl_roles = $1 WHERE channel_id = $2", pqxx::params(*new_wl_roles, channel_id));
		updated = true;
	}

	if(updated){
		std::vector<int> wl_users = resource_utils::parse_psql_int_array(ch_row[0]["wl_users"]),
				 wl_roles = resource_utils::parse_psql_int_array(ch_row[0]["wl_roles"]);
		ch_row = tx.exec("SELECT * FROM channels WHERE channel_id = $1", pqxx::params(channel_id));
		pqxx::result role_rows = tx.exec("SELECT role_id, perms1 FROM channel_x_role "
						 "WHERE channel_id = $1",
						 pqxx::params(channel_id));
		socket_event ev;
		ev.data = json_utils::channel_from_row(ch_row[0], &role_rows);
		json_utils::set_ids(ev.data, server_id);

		if(new_wl_users || new_wl_roles){ // whitelist changed
			// Get server roles that have PERM1_MANAGE_CHANNELS
			std::vector<int> ch_manage_roles;
			std::vector<pqxx::row> server_roles = role_utils::get_role_list(tx, server_id);
			int cur_ch_manage_bits = 0;
			for(auto i = server_roles.rbegin(); i != server_roles.rend(); ++i){
				int ch_manage_bits = GET_PERM_BIT((*i)["perms1"].as<long long>(), PERM1_MANAGE_CHANNELS);
				if(ch_manage_bits != 0)
					cur_ch_manage_bits = ch_manage_bits;

				if(cur_ch_manage_bits == 1)
					ch_manage_roles.push_back((*i)["role_id"].as<int>());
			}

			if( !(new_wl_users ? new_wl_users->size() : wl_users.size()) &&
			    !(new_wl_roles ? new_wl_roles->size() : wl_roles.size()) ){
				// whitelist -> no whitelist
				std::vector<int> old_users;
				get_old_wl_users(wl_users, wl_roles,
							server_id, tx, old_users);
				if(old_users.size()){
					ev.name = "channel_edited";
					sserv.send_to_server(server_id, tx, ev, -1,
								old_users, ch_manage_roles);
				}
				ev.name = "channel_created";
				sserv.send_to_server(server_id, tx, ev, -1,
							{}, {},
							old_users, ch_manage_roles);
			} else if(!wl_users.size() && !wl_roles.size() &&
					((new_wl_users && new_wl_users->size()) || (new_wl_roles && new_wl_roles->size()))){
				// no whitelist -> whitelist
				std::vector<int> new_users;
				get_new_wl_users(wl_users, wl_roles, new_wl_users.get(), new_wl_roles.get(),
							server_id, tx, new_users);
				if(new_users.size()){
					ev.name = "channel_edited";
					sserv.send_to_server(server_id, tx, ev, -1,
								new_users, ch_manage_roles);
				}
				ev.name = "channel_deleted";
				ev.data = {
					{"id", channel_id},
					{"server_id", server_id}
				};
				sserv.send_to_server(server_id, tx, ev, -1,
							{}, {}, new_users, ch_manage_roles);
			} else {
				// whitelist -> whitelist
				std::vector<int> old_users, new_users;
				get_old_wl_users(wl_users, wl_roles,
							server_id, tx, old_users);
				get_new_wl_users(wl_users, wl_roles, new_wl_users.get(), new_wl_roles.get(),
							server_id, tx, new_users);

				array_diff<int> diff(old_users, new_users);
				if(diff.unchanged.size()){
					ev.name = "channel_edited";
					sserv.send_to_server(server_id, tx, ev, -1,
								diff.unchanged, ch_manage_roles);
				}
				if(diff.added.size()){
					ev.name = "channel_created";
					sserv.send_to_server(server_id, tx, ev, -1,
								diff.added, {},
								{}, ch_manage_roles);
				}
				if(diff.removed.size()){
					ev.name = "channel_deleted";
					ev.data = {
						{"id", channel_id},
						{"server_id", server_id}
					};
					sserv.send_to_server(server_id, tx, ev, -1,
								diff.removed, {},
								{}, ch_manage_roles);
				}
			}
		} else {
			ev.name = "channel_edited";
			sserv.send_to_server(server_id, tx, ev, -1,
						wl_users, wl_roles);
		}

		tx.commit();
	}

	return create_response::string(req, "Changed", 200);
}
std::shared_ptr<http_response> channel_resource::render_DELETE(const http_request& req)
{
	base_resource::render_DELETE(req);

	int user_id, channel_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_channel_id(req, tx, user_id, server_id, channel_id);
	if(err) return err;

	if(server_id == -1)
		return create_response::string(req, "Cannot delete a DM channel", 403);

	err = role_utils::check_permission(req, tx, server_id, user_id,
						role_utils::perms1, PERM1_MANAGE_CHANNELS);
	if(err) return err;

	pqxx::result r = tx.exec("DELETE FROM channels WHERE channel_id = $1 RETURNING wl_users, wl_roles",
				 pqxx::params(channel_id));
	tx.commit();

	std::vector<int> wl_users = resource_utils::parse_psql_int_array(r[0]["wl_users"]),
			 wl_roles = resource_utils::parse_psql_int_array(r[0]["wl_roles"]);
	socket_event ev;
	ev.data["id"] = channel_id;
	json_utils::set_ids(ev.data, server_id);
	ev.name = "channel_deleted";
	sserv.send_to_server(server_id, tx, ev, -1,
				wl_users, wl_roles);

	return create_response::string(req, "Deleted", 200);
}


channel_user_id_resource::channel_user_id_resource(webserver& ws, db_connection_pool& pool, const config& cfg,
						socket_vc_server& vcserv):
	base_resource(ws, "/channels/{channel_id}/users/{user_id}", pool, cfg),
	vcserv{vcserv}
{
	set_allowing("DELETE", true);
}

std::shared_ptr<http_response> channel_user_id_resource::render_DELETE(const http_request& req)
{
	base_resource::render_DELETE(req);

	int user_id, channel_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_channel_id(req, tx, user_id, server_id, channel_id);
	if(err) return err;

	int other_user_id;
	err = resource_utils::parse_user_id(req, tx, other_user_id);
	if(err) return err;

	pqxx::result r = tx.exec("SELECT type FROM channels WHERE channel_id = $1", pqxx::params(channel_id));
	if(r[0]["type"].as<int>() != CHANNEL_VOICE)
		return create_response::string(req, "Not a voice channel", 400);

	if(server_id > -1){
		err = role_utils::check_permission(req, tx, server_id, user_id,
							role_utils::perms1, PERM1_MANAGE_VC);
		if(err) return err;
	} else {
		// If user themselves are calling/in the call, forbid kicking the other participant
		if(vcserv.has_user(channel_id, user_id))
			return create_response::string(req, "Cannot kick the other user while in the call", 403);
	}

	if(vcserv.kick_user(channel_id, other_user_id, server_id > -1 ? "Kicked by administrator" : "Call denied"))
		return create_response::string(req, "Kicked", 200);
	return create_response::string(req, "User is not in this voice channel", 404);
}

channel_roles_resource::channel_roles_resource(webserver& ws, db_connection_pool& pool, const config& cfg,
						socket_main_server& sserv):
	base_resource(ws, "/channels/{channel_id}/roles/{server_role_id}", pool, cfg),
	sserv{sserv}
{
	set_allowing("POST", true);
	set_allowing("DELETE", true);
}

std::shared_ptr<http_response> channel_roles_resource::render_POST(const http_request& req)
{
	base_resource::render_POST(req);

	int user_id, channel_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_channel_id(req, tx, user_id, server_id, channel_id);
	if(err) return err;

	if(server_id == -1)
		return create_response::string(req, "Cannot manage a DM channel", 403);

	int server_role_id;
	err = role_utils::parse_server_role_id(req, server_id, tx, server_role_id);
	if(err) return err;

	err = role_utils::check_permission(req, tx, server_id, user_id,
						role_utils::perms1, PERM1_MANAGE_CHANNELS);
	if(err) return err;


	nlohmann::json body;
	err = json_utils::from_content(req, body);
	if(err)
		return err;

	pqxx::result role_rows = tx.exec("SELECT perms1 FROM channel_x_role "
					 "WHERE channel_id = $1 AND role_id = $2",
					 pqxx::params(channel_id, server_role_id));
	long long prev_perms1 = role_rows.size() ? role_rows[0]["perms1"].as<long long>() : 0;

	JSON_GET_UNSIGNED(body, perms1)
	err = role_utils::check_permission_validity(req, tx,
							role_utils::perms1,
							prev_perms1, perms1,
							server_id, user_id);
	if(err)
		return err;

	tx.exec("INSERT INTO channel_x_role(channel_id, role_id, perms1) "
		"VALUES($1, $2, $3) "
		"ON CONFLICT(channel_id, role_id) DO UPDATE "
		"SET perms1 = $3",
		pqxx::params(channel_id, server_role_id, perms1));

	pqxx::result ch_row = tx.exec("SELECT * FROM channels WHERE channel_id = $1", pqxx::params(channel_id));
	role_rows = tx.exec("SELECT role_id, perms1 FROM channel_x_role "
					 "WHERE channel_id = $1",
					 pqxx::params(channel_id));
	tx.commit();

	socket_event ev;
	ev.data = json_utils::channel_from_row(ch_row[0], &role_rows);
	json_utils::set_ids(ev.data, server_id);
	ev.name = "channel_edited";
	sserv.send_to_channel(channel_id, tx, ev);

	return create_response::string(req, "Changed", 200);
}
std::shared_ptr<http_response> channel_roles_resource::render_DELETE(const http_request& req)
{
	base_resource::render_DELETE(req);

	int user_id, channel_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_channel_id(req, tx, user_id, server_id, channel_id);
	if(err) return err;

	if(server_id == -1)
		return create_response::string(req, "Cannot manage a DM channel", 403);

	int server_role_id;
	err = role_utils::parse_server_role_id(req, server_id, tx, server_role_id);
	if(err) return err;

	err = role_utils::check_permission(req, tx, server_id, user_id,
						role_utils::perms1, PERM1_MANAGE_CHANNELS);
	if(err) return err;

	tx.exec("DELETE FROM channel_x_role "
		"WHERE channel_id = $1 AND role_id = $2",
		pqxx::params(channel_id, server_role_id));

	pqxx::result ch_row = tx.exec("SELECT * FROM channels WHERE channel_id = $1", pqxx::params(channel_id));
	pqxx::result role_rows = tx.exec("SELECT role_id, perms1 FROM channel_x_role "
					 "WHERE channel_id = $1",
					 pqxx::params(channel_id));
	tx.commit();

	socket_event ev;
	ev.data = json_utils::channel_from_row(ch_row[0], &role_rows);
	json_utils::set_ids(ev.data, server_id);
	ev.name = "channel_edited";
	sserv.send_to_channel(channel_id, tx, ev);

	return create_response::string(req, "Changed", 200);

}
