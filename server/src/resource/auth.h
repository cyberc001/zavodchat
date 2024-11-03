#ifndef RESOURCE_AUTH_H
#define RESOURCE_AUTH_H

#include "db/conn_pool.h"
#include <unordered_map>
#include <random>

#include <httpserver.hpp>
using namespace httpserver;

typedef uint64_t session_token;

// Assumes that the calling class has field {auth_resourse& auth} and macro is called inside a render method
#define PARSE_SESSION_TOKEN(var, token_string)\
	try {\
		(var) = std::stoull(token_string);\
	} catch(std::invalid_argument& e) {\
		return std::shared_ptr<http_response>(new string_response("Invalid token", 400));\
	}\
	if(auth.sessions.find(var) == auth.sessions.end())\
		return std::shared_ptr<http_response>(new string_response("Expired or invalid token", 403));\

class auth_resource : public http_resource
{
public:
	auth_resource(connection_pool& pool);
	std::shared_ptr<http_response> render_POST(const http_request&);
	std::shared_ptr<http_response> render_PUT(const http_request&);

	session_token generate_session_token();
	std::unordered_map<session_token, int> sessions; // maps session tokens to user_id in table users
private:
	connection_pool& pool;

	std::random_device seed_gen;
	std::mt19937 rand_gen;
	std::uniform_int_distribution<session_token> rand_distribution;
};

#endif
