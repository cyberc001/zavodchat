#include "resource/utils/channel.h"
#include "resource/utils/role.h"
#include "resource/utils.h"

int channel_utils::find_head(pqxx::work& tx, int server_id)
{
	pqxx::result r = tx.exec("SELECT head_channel_id FROM servers "
				 "WHERE server_id = $1",
				 pqxx::params(server_id));
	if(!r.size())
		return -2;
	return r[0]["head_channel_id"].as<int>();
}

std::string channel_utils::insert(pqxx::work& tx, int server_id,
					std::string name, int type,
					int prev_channel_id,
					const std::vector<int>& wl_users, const std::vector<int> wl_roles,
					pqxx::row* out)
{
	pqxx::row _out;
	if(out == nullptr)
		out = &_out;

	if(prev_channel_id != -1 && !check_member(-1, prev_channel_id, server_id, tx))
		return "prev_channel_id does not exist";

	try{
		std::string wl_columns, wl_params;
		pqxx::params pr(server_id, name, type, prev_channel_id);
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

		*out = tx.exec("INSERT INTO channels(server_id, name, type, prev_channel_id" + wl_columns + ") VALUES($1, $2, $3, $4" + wl_params +
			       ") RETURNING channel_id, name, type, wl_users, wl_roles",
			       pr)[0];
	} catch(pqxx::data_exception& e){
		return "Channel name is too long";
	}

	int channel_id = (*out)["channel_id"].as<int>();
	int head = find_head(tx, server_id);
	if(head == -1 || head == prev_channel_id)
		tx.exec("UPDATE servers SET head_channel_id = $1 "
			"WHERE server_id = $2",
			pqxx::params(channel_id, server_id));
	else
		tx.exec("UPDATE channels SET prev_channel_id = $1 "
			"WHERE server_id = $2 AND prev_channel_id = $3 AND channel_id != $1",
			pqxx::params(channel_id, server_id, prev_channel_id));

	return "";
}
std::shared_ptr<http_response> channel_utils::insert(const http_request& req, pqxx::work& tx, int server_id,
							std::string name, int type,
							int prev_channel_id,
							const std::vector<int>& wl_users, const std::vector<int> wl_roles,
							pqxx::row* out)
{
	std::string err = insert(tx, server_id, name, type, prev_channel_id, wl_users, wl_roles, out);
	if(err.size())
		return create_response::string(req, err, 400);
	return nullptr;
}


std::string channel_utils::move(pqxx::work& tx, int server_id, int channel_id,
							int prev_channel_id)
{
	if(prev_channel_id == channel_id)
		return "prev_channel_id should not be equal to channel_id";

	pqxx::result r = tx.exec("SELECT * FROM channels WHERE channel_id = $1", channel_id);

	int head_channel_id = channel_utils::find_head(tx, server_id);
	int old_prev_channel_id = r[0]["prev_channel_id"].as<int>();

	// Remove channel from linked list
	tx.exec("UPDATE channels SET prev_channel_id = $1 "
		"WHERE prev_channel_id = $2",
		pqxx::params(old_prev_channel_id, channel_id));

	// Insert channel back into the linked list
	if(prev_channel_id == -1 && !check_member(-1, channel_id, server_id, tx))
		return "prev_channel_id does not exist";
	tx.exec("UPDATE channels SET prev_channel_id = $1 "
		"WHERE prev_channel_id = $2",
		pqxx::params(channel_id, prev_channel_id));
	if(head_channel_id == prev_channel_id)
		tx.exec("UPDATE servers SET head_channel_id = $1 "
			"WHERE server_id = $2",
			pqxx::params(channel_id, server_id));
	else if(head_channel_id == channel_id)
		tx.exec("UPDATE servers SET head_channel_id = $1 "
			"WHERE server_id = $2",
			pqxx::params(old_prev_channel_id, server_id));

	// Update the channel itself
	tx.exec("UPDATE channels SET prev_channel_id = $1 "
		"WHERE channel_id = $2",
		pqxx::params(prev_channel_id, channel_id));

	return "";
}
std::shared_ptr<http_response> channel_utils::move(const http_request& req, pqxx::work& tx, int server_id, int channel_id,
							int prev_channel_id)
{
	std::string err = move(tx, server_id, channel_id, prev_channel_id);
	if(err.size())
		return create_response::string(req, err, 400);
	return nullptr;
}

int channel_utils::get_other_user_id(int channel_id, int user_id, pqxx::work& tx)
{
	pqxx::result r = tx.exec("SELECT user1_id, user2_id FROM channels WHERE channel_id=$1", pqxx::params(channel_id));
	int user1_id = r[0]["user1_id"].as<int>(), user2_id = r[0]["user2_id"].as<int>();
	return user_id == user1_id ? user2_id : user1_id;
}
bool channel_utils::check_member(int user_id, int channel_id, int server_id, pqxx::work& tx)
{
	if(user_id == -1){
		pqxx::result ch = tx.exec("SELECT channel_id FROM channels "
					  "WHERE channel_id = $1 AND server_id = $2",
					  pqxx::params(channel_id, server_id));
		return ch.size();
	}

	if(!role_utils::check_permission(tx, server_id, user_id,
						role_utils::perms1, PERM1_MANAGE_CHANNELS)){
		pqxx::result ch = tx.exec("SELECT wl_users, wl_roles FROM channels WHERE channel_id = $1",
					     pqxx::params(channel_id));
		if(!ch.size())
			return false;
		std::vector<int> wl_users = resource_utils::parse_psql_int_array(ch[0]["wl_users"]);
		if(wl_users.size() > 0 && std::find(wl_users.begin(), wl_users.end(), user_id) != wl_users.end())
			return true;

		std::vector<int> wl_roles = resource_utils::parse_psql_int_array(ch[0]["wl_roles"]);
		if(wl_users.size() == 0 && wl_roles.size() == 0)
			return true;
		pqxx::result roles = tx.exec("SELECT role_id FROM user_x_server WHERE user_id = $1 AND server_id = $2",
					     pqxx::params(user_id, server_id));
		for(size_t i = 0; i < roles.size(); ++i)
			if(std::find(wl_roles.begin(), wl_roles.end(), roles[i]["role_id"].as<int>()) != wl_roles.end())
				return true;

		return false;
	}
	return true;
}

std::vector<int> channel_utils::get_users(int channel_id, pqxx::work& tx, int user_id)
{
	std::vector<int> out;
	pqxx::result r = tx.exec("SELECT server_id, user1_id, user2_id, wl_users, wl_roles FROM channels WHERE channel_id = $1",
				 pqxx::params(channel_id));
	if(r[0]["server_id"].is_null()){
		out.push_back(r[0]["user1_id"].as<int>());
		out.push_back(r[0]["user2_id"].as<int>());
	} else {
		int server_id = r[0]["server_id"].as<int>();
		std::vector<int> wl_users = resource_utils::parse_psql_int_array(r[0]["wl_users"]),
				 wl_roles = resource_utils::parse_psql_int_array(r[0]["wl_roles"]);

		std::string wl_check, having;
		if(wl_users.size() > 0 || wl_roles.size() > 0){
			wl_check = "AND (";
			if(wl_users.size() > 0)
				wl_check += "user_id IN (" + resource_utils::int_array_to_string(wl_users) + ")";
			if(wl_roles.size() > 0)
				wl_check += std::string(wl_users.size() > 0 ? " OR " : "") + "role_id IN (" + resource_utils::int_array_to_string(wl_roles) + ")";
			wl_check += ")";
		}

		pqxx::params pr(server_id);
		if(user_id > -1)
			pr.append(user_id);
		r = tx.exec("SELECT user_id FROM user_x_server "
			    "WHERE server_id = $1 " + wl_check + 
			    (user_id > -1 ? " AND NOT EXISTS(SELECT user2_id FROM blocked_users WHERE user1_id = user_id AND user2_id = $2)" : "") +
			    " GROUP BY user_id" + having,
			    pr);
		for(size_t i = 0; i < r.size(); ++i)
			out.push_back(r[i]["user_id"].as<int>());
	}
	return out;
}
