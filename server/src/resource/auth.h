#ifndef RESOURCE_AUTH_H
#define RESOURCE_AUTH_H

#include "db/conn_pool.h"
#include <unordered_map>
#include <random>

#include <httpserver.hpp>
using namespace httpserver;

typedef uint64_t session_token;

class auth_resource : public http_resource
{
public:
	auth_resource(db_connection_pool& pool);
	std::shared_ptr<http_response> render_POST(const http_request&);
	std::shared_ptr<http_response> render_PUT(const http_request&);

	session_token generate_session_token();
	std::shared_ptr<http_response> parse_session_token(const http_request&, session_token& st); // returns nullptr if there was no error parsing token
	std::unordered_map<session_token, int> sessions; // maps session tokens to user_id in table users

	unsigned min_username_length = 2;
	unsigned min_password_length = 8;
private:
	db_connection_pool& pool;

	std::random_device seed_gen;
	std::mt19937 rand_gen;
	std::uniform_int_distribution<session_token> rand_distribution;
};

#endif
