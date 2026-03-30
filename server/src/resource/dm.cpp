#include "resource/dm.h"
#include "resource/utils.h"
#include "resource/channel.h"

dm_resource::dm_resource(webserver& ws, db_connection_pool& pool, const config& cfg,
				socket_vc_server& vcserv):
	base_resource(ws, "/dms", pool, cfg),
	vcserv{vcserv}
{
	set_allowing("GET", true);
}

std::shared_ptr<http_response> dm_resource::render_GET(const http_request& req)
{
	base_resource::render_GET(req);

	db_connection conn = pool.hold();
	pqxx::work tx{*conn};

	int user_id;
	auto err = resource_utils::parse_session_token(req, tx, user_id);
	if(err) return err;

	pqxx::params pr(user_id);
	std::string pg_query;
	err = resource_utils::pagination_query(req, cfg, "message_id", pr, pg_query);
	if(err)
		return err;

	nlohmann::json res = nlohmann::json::array();
	pqxx::result r = tx.exec("SELECT * FROM channels LEFT JOIN LATERAL (SELECT * FROM messages WHERE messages.channel_id = channels.channel_id ORDER BY message_id DESC LIMIT 1) ON true WHERE type = 0 AND (user1_id = $1 OR user2_id = $1)" + pg_query, pr);

	for(size_t i = 0; i < r.size(); ++i){
		nlohmann::json channel_json = resource_utils::channel_json_from_row(r[i]);
		int user1_id = r[i]["user1_id"].as<int>(), user2_id = r[i]["user2_id"].as<int>();
		channel_json["user_id"] = user1_id == user_id ? user2_id : user1_id;
		if(!r[i]["message_id"].is_null()){
			pqxx::result attachment_rows;
			channel_json["last_message"] = resource_utils::message_json_from_row(r[i], attachment_rows);
		}
		//if(channel_json["type"] == CHANNEL_VOICE)
		//	channel_json["vc_users"] = vcserv.get_channel_users(channel_json["id"]);
		res += channel_json;
	}

	return create_response::string(req, res.dump(), 200);
}


dm_id_resource::dm_id_resource(webserver& ws, db_connection_pool& pool, const config& cfg):
	base_resource(ws, "/dms/{user_id}", pool, cfg)
{
	set_allowing("POST", true);
}

std::shared_ptr<http_response> dm_id_resource::render_POST(const http_request& req)
{
	base_resource::render_GET(req);

	db_connection conn = pool.hold();
	pqxx::work tx{*conn};

	int user_id;
	auto err = resource_utils::parse_session_token(req, tx, user_id);
	if(err) return err;

	int other_user_id;
	err = resource_utils::parse_user_id(req, tx, other_user_id);
	if(err) return err;

	pqxx::result r = tx.exec("SELECT * FROM channels WHERE (user1_id = $1 AND user2_id = $2) OR (user1_id = $2 AND user2_id = $1)", pqxx::params(user_id, other_user_id));
	if(!r.size()){
		r = tx.exec("INSERT INTO channels(user1_id, user2_id, type) VALUES($1, $2, 0), ($1, $2, 1) RETURNING channel_id", pqxx::params(user_id, other_user_id));
	}
	tx.commit();

	nlohmann::json res = nlohmann::json::array();
	res += r[0]["channel_id"].as<int>();
	res += r[1]["channel_id"].as<int>();
	return create_response::string(req, res.dump(), 200);
}
