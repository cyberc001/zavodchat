#ifndef UTILS_CHANNEL_H
#define UTILS_CHANNEL_H

#include <pqxx/pqxx>
#include <vector>
#include <httpserver.hpp>
using namespace httpserver;

enum channel_type {
	CHANNEL_TEXT = 0,
	CHANNEL_VOICE
};

class channel_utils
{
public:
	static int find_head(pqxx::work&, int server_id);

	static std::string insert(pqxx::work& tx, int server_id,
					std::string name, int type,
					int prev_channel_id = -1,
					const std::vector<int>& wl_users = {}, const std::vector<int> wl_roles = {},
					pqxx::row* out = nullptr);
	static std::shared_ptr<http_response> insert(const http_request& req, pqxx::work& tx, int server_id,
					std::string name, int type,
					int prev_channel_id = -1,
					const std::vector<int>& wl_users = {}, const std::vector<int> wl_roles = {},
					pqxx::row* out = nullptr);

	static std::string move(pqxx::work& tx, int server_id, int channel_id,
					int prev_channel_id);
	static std::shared_ptr<http_response> move(const http_request& req, pqxx::work& tx, int server_id, int channel_id,
							int prev_channel_id);



	static int get_other_user_id(int channel_id, int user_id, pqxx::work&);

	// If user_id == -1, just checks if channel exists and server matches
	static bool check_member(int user_id, int channel_id, int server_id, pqxx::work&);

	// Get users that have access to this channel. If user_id != -1, don't add blocked users
	static std::vector<int> get_users(int channel_id, pqxx::work&, int user_id = -1);
};

#endif
