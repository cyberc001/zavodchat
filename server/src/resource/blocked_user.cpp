#include "resource/blocked_user.h"
#include "resource/utils.h"

blocked_users_resource::blocked_users_resource(webserver& ws, db_connection_pool& pool, const config& cfg):
	base_resource(ws, "/blocked_users", pool, cfg)
{
	set_allowing("GET", true);
}

std::shared_ptr<http_response> blocked_users_resource::render_GET(const http_request& req)
{
	base_resource::render_GET(req);

	db_connection conn = pool.hold();
	pqxx::work tx{*conn};

	int user_id;
	auto err = resource_utils::parse_session_token(req, tx, user_id);
	if(err) return err;

	nlohmann::json res = nlohmann::json::array();
	pqxx::result r = tx.exec("SELECT user_id, name, avatar, status FROM (SELECT user2_id FROM blocked_users WHERE user1_id = $1) JOIN users ON user2_id = user_id", pqxx::params(user_id));
	for(size_t i = 0; i < r.size(); ++i)
		res += resource_utils::user_json_from_row(r[i]);
	return create_response::string(req, res.dump(), 200);
}


blocked_users_id_resource::blocked_users_id_resource(webserver& ws, db_connection_pool& pool, const config& cfg):
	base_resource(ws, "/blocked_users/{user_id}", pool, cfg)
{
	set_allowing("POST", true);
	set_allowing("DELETE", true);
}

std::shared_ptr<http_response> blocked_users_id_resource::render_POST(const http_request& req)
{
	base_resource::render_POST(req);

	db_connection conn = pool.hold();
	pqxx::work tx{*conn};

	int user_id;
	auto err = resource_utils::parse_session_token(req, tx, user_id);
	if(err) return err;

	int block_user_id;
	err = resource_utils::parse_user_id(req, tx, block_user_id);
	if(err) return err;

	if(user_id == block_user_id)
		return create_response::string(req, "Cannot block yourself", 400);

	pqxx::result r = tx.exec("SELECT user2_id FROM blocked_users WHERE user1_id = $1 AND user2_id = $2", pqxx::params(user_id, block_user_id));
	if(r.size())
		return create_response::string(req, "User is already blocked", 202);

	r = tx.exec("SELECT user1_id FROM blocked_users WHERE user1_id = $1", pqxx::params(user_id));
	if(r.size() >= cfg.max_blocked_per_user)
		return create_response::string(req, "Too many blocked users", 403);

	tx.exec("INSERT INTO blocked_users VALUES($1, $2)", pqxx::params(user_id, block_user_id));
	tx.exec("DELETE FROM friends WHERE (user1_id = $1 AND user2_id = $2) OR (user1_id = $2 AND user1_id = $1)", pqxx::params(user_id, block_user_id));
	tx.commit();
	return create_response::string(req, "User was blocked", 200);
}

std::shared_ptr<http_response> blocked_users_id_resource::render_DELETE(const http_request& req)
{
	base_resource::render_DELETE(req);

	db_connection conn = pool.hold();
	pqxx::work tx{*conn};

	int user_id;
	auto err = resource_utils::parse_session_token(req, tx, user_id);
	if(err) return err;

	int block_user_id;
	err = resource_utils::parse_user_id(req, tx, block_user_id);
	if(err) return err;

	pqxx::result r = tx.exec("SELECT user2_id FROM blocked_users WHERE user1_id = $1 AND user2_id = $2", pqxx::params(user_id, block_user_id));
	if(!r.size())
		return create_response::string(req, "User is already unblocked", 202);

	tx.exec("DELETE from blocked_users WHERE user1_id = $1 AND user2_id = $2", pqxx::params(user_id, block_user_id));
	tx.commit();
	return create_response::string(req, "User was unblocked", 200);
}
