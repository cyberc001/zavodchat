#include "resource/friend.h"
#include "resource/utils.h"

void send_friend_id_event(pqxx::work& tx, socket_main_server& sserv,
			int user_id_from, int user_id_to, std::string name)
{
	socket_event ev;
	ev.data["id"] = user_id_from;
	ev.name = name;
	sserv.send_to_user(user_id_to, tx, ev);
}


friends_resource::friends_resource(webserver& ws, db_connection_pool& pool, const config& cfg):
	base_resource(ws, "/friends", pool, cfg)
{
	set_allowing("GET", true);
}

std::shared_ptr<http_response> friends_resource::render_GET(const http_request& req)
{
	base_resource::render_GET(req);

	db_connection conn = pool.hold();
	pqxx::work tx{*conn};

	int user_id;
	auto err = resource_utils::parse_session_token(req, tx, user_id);
	if(err) return err;

	nlohmann::json res = nlohmann::json::array();
	pqxx::result r = tx.exec("SELECT user1_id, user2_id FROM friends WHERE (user1_id = $1 OR user2_id = $1) AND NOT is_request", pqxx::params(user_id));
	for(size_t i = 0; i < r.size(); ++i)
		res += r[i]["user1_id"].as<int>() == user_id ? r[i]["user2_id"].as<int>() : r[i]["user1_id"].as<int>();
	return create_response::string(req, res.dump(), 200);
}

friend_requests_resource::friend_requests_resource(webserver& ws, db_connection_pool& pool, const config& cfg):
	base_resource(ws, "/friend_requests", pool, cfg)
{
	set_allowing("GET", true);
}

std::shared_ptr<http_response> friend_requests_resource::render_GET(const http_request& req)
{
	base_resource::render_GET(req);

	db_connection conn = pool.hold();
	pqxx::work tx{*conn};

	int user_id;
	auto err = resource_utils::parse_session_token(req, tx, user_id);
	if(err) return err;

	int incoming = 1;
	auto args = req.get_args();
	if(args.find(std::string_view("incoming")) != args.end()){
		auto err = resource_utils::parse_index(req, "incoming", incoming);
		if(err) return err;
		if(incoming != 0 && incoming != 1)
			return create_response::string(req, "Invalid 'incoming' value", 400);
	}

	nlohmann::json res = nlohmann::json::array();
	pqxx::result r = tx.exec("SELECT user1_id, user2_id FROM friends WHERE " + std::string(incoming ? "user2_id" : "user1_id") + " = $1 AND is_request", pqxx::params(user_id));
	for(size_t i = 0; i < r.size(); ++i)
		res += r[i]["user1_id"].as<int>() == user_id ? r[i]["user2_id"].as<int>() : r[i]["user1_id"].as<int>();
	return create_response::string(req, res.dump(), 200);
}

friends_id_resource::friends_id_resource(webserver& ws, db_connection_pool& pool, const config& cfg,
						socket_main_server& sserv):
	base_resource(ws, "/friends/{user_id}", pool, cfg),
	sserv{sserv}
{
	set_allowing("POST", true);
	set_allowing("DELETE", true);
}

std::shared_ptr<http_response> friends_id_resource::render_POST(const http_request& req)
{
	base_resource::render_POST(req);

	db_connection conn = pool.hold();
	pqxx::work tx{*conn};

	int user_id;
	auto err = resource_utils::parse_session_token(req, tx, user_id);
	if(err) return err;

	int friend_user_id;
	err = resource_utils::parse_user_id(req, tx, friend_user_id);
	if(err) return err;

	if(user_id == friend_user_id)
		return create_response::string(req, "Cannot add yourself as a friend", 400);

	pqxx::result r = tx.exec("SELECT user1_id, user2_id, is_request FROM friends WHERE (user1_id = $1 AND user2_id = $2) OR (user1_id = $2 AND user2_id = $1)", pqxx::params(user_id, friend_user_id));
	// If users are not friends and dont have any requests to each other, create one
	if(!r.size()){
		err = resource_utils::check_user_unblocked(req, user_id, friend_user_id, tx);
		if(err) return err;

		r = tx.exec("SELECT user1_id FROM friends WHERE user1_id = $1 OR user2_id = $1", pqxx::params(user_id));
		if(r.size() >= cfg.max_friends_per_user)
			return create_response::string(req, "Too many friends", 403);

		tx.exec("INSERT INTO friends(user1_id, user2_id, is_request) VALUES($1, $2, true)", pqxx::params(user_id, friend_user_id));
		tx.commit();
		send_friend_id_event(tx, sserv, user_id, friend_user_id, "friend_request_received");
		return create_response::string(req, "Friend request was sent", 200);
	}

	if(!r[0]["is_request"].as<bool>())
		return create_response::string(req, "User is already a friend", 202);
	// Check request direction (from whom?)
	if(r[0]["user1_id"].as<int>() == user_id) // From user
		return create_response::string(req, "Friend request was already sent", 202);
	// From friend_user_id: accept it
	tx.exec("UPDATE friends SET is_request=false WHERE user1_id = $1 AND user2_id = $2", pqxx::params(friend_user_id, user_id));
	tx.commit();
	send_friend_id_event(tx, sserv, user_id, friend_user_id, "friend_request_accepted");
	return create_response::string(req, "Friend request was accepted", 200);
}

std::shared_ptr<http_response> friends_id_resource::render_DELETE(const http_request& req)
{
	base_resource::render_DELETE(req);

	db_connection conn = pool.hold();
	pqxx::work tx{*conn};

	int user_id;
	auto err = resource_utils::parse_session_token(req, tx, user_id);
	if(err) return err;

	int friend_user_id;
	err = resource_utils::parse_user_id(req, tx, friend_user_id);
	if(err) return err;

	pqxx::result r = tx.exec("SELECT user1_id, user2_id, is_request FROM friends WHERE (user1_id = $1 AND user2_id = $2) OR (user1_id = $2 AND user2_id = $1)", pqxx::params(user_id, friend_user_id));
	if(!r.size())
		return create_response::string(req, "User is not a friend, and no friend request was sent", 403);
	tx.exec("DELETE FROM friends WHERE user1_id = $1 AND user2_id = $2", pqxx::params(r[0]["user1_id"].as<int>(), r[0]["user2_id"].as<int>()));
	tx.commit();

	if(r[0]["is_request"].as<bool>()){
		if(r[0]["user1_id"].as<int>() == user_id){
			send_friend_id_event(tx, sserv, user_id, friend_user_id, "friend_request_cancelled");
			return create_response::string(req, "Cancelled friend request", 200);
		} else {
			send_friend_id_event(tx, sserv, user_id, friend_user_id, "friend_request_denied");
			return create_response::string(req, "Denied friend request", 200);
		}
	}
	send_friend_id_event(tx, sserv, user_id, friend_user_id, "friend_removed");
	return create_response::string(req, "Removed friend", 200);
}
