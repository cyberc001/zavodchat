#ifndef RESOURCE_SERVER_H
#define RESOURCE_SERVER_H

#include "db/conn_pool.h"
#include "resource/auth.h"

class server_resource : public http_resource
{
public:
	server_resource(connection_pool& pool, auth_resource& auth);
	std::shared_ptr<http_response> render_GET(const http_request&);
	std::shared_ptr<http_response> render_POST(const http_request&);

	unsigned owned_per_user = 2;
private:
	connection_pool& pool;
	auth_resource& auth;
};

class server_id_resource : public http_resource
{
public:
	server_id_resource(connection_pool& pool, auth_resource& auth);
	std::shared_ptr<http_response> render_GET(const http_request&);
	std::shared_ptr<http_response> render_DELETE(const http_request&);

	// Checks if the server is accessible to the user. Returns nullptr if there wasn't an error
	static std::shared_ptr<http_response> parse_id(const http_request&, int user_id, pqxx::work&, int& server_id);
	// Checks if server's owner_id == user_id. Returns nullptr if it's true
	static std::shared_ptr<http_response> check_owner(int user_id, int server_id, pqxx::work&);
private:
	connection_pool& pool;
	auth_resource& auth;
};

#endif
