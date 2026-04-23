#include "resource/user.h"
#include "resource/utils.h"
#include "resource/json_utils.h"

user_resource::user_resource(webserver& ws, db_connection_pool& pool, const config& cfg):
	base_resource(ws, "/users", pool, cfg)
{
	set_allowing("GET", true);
}

std::shared_ptr<http_response> user_resource::render_GET(const http_request& req)
{
	base_resource::render_GET(req);

	db_connection conn = pool.hold();
	pqxx::work tx{*conn};

	int start_id;
	auto err = resource_utils::parse_index(req, "start_id", start_id, 0);
	if(err) return err;
	int count;
	err = resource_utils::parse_index(req, "count", count, 0, cfg.max_get_count);
	if(err) return err;
	std::string order;
	err = resource_utils::parse_order(req, order);
	if(err) return err;

	int user_id;
	auto invalid_user_id = resource_utils::parse_session_token(req, tx, user_id);

	pqxx::params pr(start_id, count);
	if(!invalid_user_id)
		pr.append(user_id);

	std::string where_displayname = "";
	auto args = req.get_args();
	if(args.find(std::string_view("displayname")) != args.end()){
		pr.append(std::string(args["displayname"]));
		where_displayname = "AND name LIKE '%' || $" + std::to_string(pr.size()) + " || '%'";
	}

	pqxx::result r = tx.exec("SELECT user_id, name, avatar, status FROM users WHERE " +
				 std::string(invalid_user_id ? "" : resource_utils::no_blocked_users_query(3) + " AND ") +
				 "user_id " + std::string(order == "DESC" ? "<=" : ">=") + " $1 " + where_displayname + " ORDER BY user_id " + order + " LIMIT $2 ",
				 pr);
	nlohmann::json res = nlohmann::json::array();
	for(size_t i = 0; i < r.size(); ++i)
		res += json_utils::user_from_row(r[i]);
	return create_response::string(req, res.dump(), 200);
}

user_id_resource::user_id_resource(webserver& ws, db_connection_pool& pool, const config& cfg):
	base_resource(ws, "/users/{user_id}", pool, cfg)
{
	set_allowing("GET", true);
}

std::shared_ptr<http_response> user_id_resource::render_GET(const http_request& req)
{
	base_resource::render_GET(req);

	db_connection conn = pool.hold();
	pqxx::work tx{*conn};

	int user_id;
	auto err = parse_id(req, tx, user_id);
	if(err) return err;

	if(user_id == -1){
		int auth_user_id;
		err = resource_utils::parse_session_token(req, tx, auth_user_id);
		if(err) return err;
		user_id = auth_user_id;
	}

	pqxx::result r = tx.exec("SELECT user_id, name, avatar, status FROM users WHERE user_id = $1",
				 pqxx::params(user_id));
	if(!r.size())
		return create_response::string(req, "User with this ID doesn't exist", 404);

	return create_response::string(req, json_utils::user_from_row(r[0]).dump(), 200);
}

std::shared_ptr<http_response> user_id_resource::parse_id(const http_request& req, pqxx::work& tx, int& user_id)
{
	try{
		user_id = std::stoi(std::string(req.get_arg("user_id")));
	} catch(std::invalid_argument& e){
		return create_response::string(req, "Invalid user ID", 400);
	}
	return nullptr;
}
