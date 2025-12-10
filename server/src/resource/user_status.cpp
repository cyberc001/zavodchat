#include <resource/user_status.h>
#include <resource/utils.h>
#include <socket/server.h>

user_status_resource::user_status_resource(db_connection_pool& pool, socket_main_server& sserv): base_resource(), pool{pool}, sserv{sserv}
{
	set_allowing("PUT", true);
}

std::shared_ptr<http_response> user_status_resource::render_PUT(const http_request& req)
{
	base_resource::render_PUT(req);

	db_connection conn = pool.hold();
	pqxx::work tx{*conn};

	int user_id;
	auto err = resource_utils::parse_session_token(req, tx, user_id);
	if(err) return err;

	int status;
	err = resource_utils::parse_index(req, "status", status, STATUS_BEGIN, STATUS_END);
	if(err) return err;

	socket_event ev;
	ev.data["id"] = user_id;
	ev.data["status"] = status;
	ev.name = "user_changed";
	sserv.send_to_user_observers(user_id, tx, ev);

	tx.exec("UPDATE users SET status = $1 WHERE user_id = $2", pqxx::params(status, user_id));
	tx.commit();

	return create_response::string(req, "Changed", 200);
}
