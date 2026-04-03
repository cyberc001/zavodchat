#include "resource/notifications.h"
#include "resource/utils.h"

notifications_resource::notifications_resource(webserver& ws, db_connection_pool& pool, const config& cfg):
	base_resource(ws, "/notifications", pool, cfg)
{
	set_allowing("GET", true);
}

std::shared_ptr<http_response> notifications_resource::render_GET(const http_request& req)
{
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};

	int user_id;
	auto err = resource_utils::parse_session_token(req, tx, user_id);
	if(err) return err;

	pqxx::result r = tx.exec("SELECT SUM(notification_count) FROM channel_notifications "
				 "JOIN channels ON channel_notifications.channel_id = channels.channel_id "
				 "WHERE user_id = $1 AND user1_id IS NOT NULL",
				 pqxx::params(user_id));
	nlohmann::json res = {
		{"dm_notifications", r[0]["sum"].is_null() ? 0 : r[0]["sum"].as<int>()}
	};

	return create_response::string(req, res.dump(), 200);
}

notification_channel_resource::notification_channel_resource(webserver& ws, db_connection_pool& pool, const config& cfg):
	base_resource(ws, "/notifications/channels/{channel_id}", pool, cfg)
{
	set_allowing("DELETE", true);
}

std::shared_ptr<http_response> notification_channel_resource::render_DELETE(const http_request& req)
{
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};

	int user_id, server_id, channel_id;
	auto err = resource_utils::parse_channel_id(req, tx, user_id, server_id, channel_id);
	if(err) return err;

	tx.exec("DELETE FROM channel_notifications WHERE channel_id=$1 AND user_id=$2", pqxx::params(channel_id, user_id));
	tx.commit();

	return create_response::string(req, "Removed notifications", 200);
}
