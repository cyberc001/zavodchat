#include <resource/user_status.h>
#include <resource/utils.h>

user_status_resource::user_status_resource(db_connection_pool& pool): base_resource(), pool{pool}
{
	set_allowing("PUT", true);
}

std::shared_ptr<http_response> user_status_resource::render_PUT(const http_request& req)
{
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};

	int user_id;
	auto err = resource_utils::parse_session_token(req, tx, user_id);
	if(err) return err;

	int status;
	err = resource_utils::parse_index(req, "status", status, STATUS_BEGIN, STATUS_END);
	if(err) return err;

	tx.exec("UPDATE users SET status = $1 WHERE user_id = $2", pqxx::params(status, user_id));

	tx.commit();
	return create_response::string(req, "Changed", 200);
}
