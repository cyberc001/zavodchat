#include "resource/emoji.h"
#include "resource/json_utils.h"
#include "resource/file_utils.h"
#include "resource/role_utils.h"

#include <cctype>

std::shared_ptr<http_response> check_emoji_name(const http_request& req, const std::string& name)
{
	if(!name.size())
		return create_response::string(req, "Empty emoji name", 400);
	for(size_t i = 0; i < name.size(); ++i)
		if(std::isspace(name[i]))
			return create_response::string(req, "Emoji contains whitespaces", 400);
		else if(name[i] == ':')
			return create_response::string(req, "Emoji contains a colon", 400);
	return nullptr;
}


server_emojis_resource::server_emojis_resource(webserver& ws, db_connection_pool& pool, const config& cfg,
						socket_main_server& sserv):
	base_resource(ws, "/servers/{server_id}/emojis", pool, cfg),
	sserv{sserv}
{
	set_allowing("GET", true);
	set_allowing("POST", true);
}

std::shared_ptr<http_response> server_emojis_resource::render_GET(const http_request& req)
{
	base_resource::render_GET(req);

	int user_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_server_id(req, tx, user_id, server_id);
	if(err) return err;

	pqxx::result r = tx.exec("SELECT * FROM emojis "
				 "WHERE server_id = $1",
				 pqxx::params(server_id));
	nlohmann::json res = nlohmann::json::array();
	for(size_t i = 0; i < r.size(); ++i)
		res += json_utils::emoji_from_row(r[i]);
	return create_response::string(req, res.dump(), 200);
}
std::shared_ptr<http_response> server_emojis_resource::render_POST(const http_request& req)
{
	base_resource::render_POST(req);

	int user_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_server_id(req, tx, user_id, server_id);
	if(err) return err;

	err = role_utils::check_permission(req, tx, server_id, user_id,
						role_utils::perms1, PERM1_MANAGE_EMOJIS);
	if(err) return err;


	std::string name = req.get_arg("name");
	err = check_emoji_name(req, name);
	if(err) return err;

	// Check emoji count and unique name
	pqxx::result r = tx.exec("SELECT * FROM emojis "
				 "WHERE server_id = $1",
				 pqxx::params(server_id));
	if(r.size() >= cfg.max_emojis_per_server)
		return create_response::string(req, "Too many emojis per server", 403);
	r = tx.exec("SELECT * FROM emojis "
		    "WHERE server_id = $1 AND name = $2",
		    pqxx::params(server_id, name));
	if(r.size())
		return create_response::string(req, "Emoji with this name already exists", 400);


	socket_event ev;
	try {
		r = tx.exec("INSERT INTO emojis(server_id, name, image) VALUES($1, $2, $3) RETURNING emoji_id",
			    pqxx::params(server_id, name, ""));
	} catch(pqxx::data_exception& e){
		return create_response::string(req, "Emoji name is too long", 400);
	}
	int emoji_id = r[0]["emoji_id"].as<int>();

	// Save image and write filename to DB
	std::string image_fname;
	err = file_utils::parse_image(req, "image", emoji_id, cfg.emoji_path, image_fname);
	if(err)
		return err;
	r = tx.exec("UPDATE emojis SET image = $1 "
		    "WHERE emoji_id = $2 "
		    "RETURNING emoji_id, name, image",
		    pqxx::params(image_fname, emoji_id));
	tx.commit();

	ev.name = "emoji_created";
	ev.data = json_utils::emoji_from_row(r[0]);
	json_utils::set_ids(ev.data, server_id);
	sserv.send_to_server(server_id, tx, ev);

	return create_response::string(req, "Created", 200);
}


emoji_id_resource::emoji_id_resource(webserver& ws, db_connection_pool& pool, const config& cfg,
						socket_main_server& sserv):
	base_resource(ws, "/servers/{server_id}/emojis/{emoji_id}", pool, cfg),
	sserv{sserv}
{
	set_allowing("PUT", true);
	set_allowing("DELETE", true);
}

std::shared_ptr<http_response> emoji_id_resource::render_PUT(const http_request& req)
{
	base_resource::render_PUT(req);

	int user_id, server_id, emoji_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_emoji_id(req, tx, user_id, server_id, emoji_id);
	if(err) return err;

	err = role_utils::check_permission(req, tx, server_id, user_id,
						role_utils::perms1, PERM1_MANAGE_EMOJIS);
	if(err) return err;

	auto args = req.get_args();
	bool changed = false;
	if(args.find(std::string_view("name")) != args.end()){
		changed = true;
		std::string name = args["name"];
		err = check_emoji_name(req, name);
		if(err) return err;

		try {
			tx.exec("UPDATE emojis SET name = $1 "
				"WHERE emoji_id = $2",
				pqxx::params(name, emoji_id));
		} catch(pqxx::data_exception& e){
			return create_response::string(req, "Emoji name is too long", 400);
		}
	}
	if(args.find(std::string_view("image")) != args.end()){
		changed = true;
		std::string image_fname;
		err = file_utils::parse_image(req, "image", emoji_id, cfg.emoji_path, image_fname);
		if(err)
			return err;

		tx.exec("UPDATE emojis SET image = $1 "
			"WHERE emoji_id = $2",
			pqxx::params(image_fname, emoji_id));
	}

	if(changed){
		socket_event ev;
		ev.data = json_utils::emoji_from_row(
				tx.exec("SELECT * FROM emojis "
					"WHERE emoji_id = $1",
					pqxx::params(emoji_id))[0]);
		json_utils::set_ids(ev.data, server_id);
		ev.name = "emoji_changed";
		sserv.send_to_server(server_id, tx, ev);
	}

	tx.commit();
	return create_response::string(req, "Changed", 200);
}

std::shared_ptr<http_response> emoji_id_resource::render_DELETE(const http_request& req)
{
	base_resource::render_DELETE(req);

	int user_id, server_id, emoji_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_emoji_id(req, tx, user_id, server_id, emoji_id);
	if(err) return err;

	pqxx::result r = tx.exec("DELETE FROM emojis "
				 "WHERE emoji_id = $1 "
				 "RETURNING image",
				 pqxx::params(emoji_id));
	tx.commit();

	std::string image_fpath = r[0]["image"].as<std::string>();
	std::string ext = image_fpath.substr(image_fpath.rfind('.') + 1);
	file_utils::delete_file_aliased(cfg.emoji_path, std::to_string(emoji_id), ext);

	socket_event ev;
	ev.data["id"] = emoji_id;
	json_utils::set_ids(ev.data, server_id);
	ev.name = "emoji_deleted";
	sserv.send_to_server(server_id, tx, ev);

	return create_response::string(req, "Deleted", 200);
}
