#ifndef ROLE_UTILS_H
#define ROLE_UTILS_H

#include <pqxx/pqxx>
#include <nlohmann/json.hpp>
#include <vector>
#include <httpserver.hpp>
using namespace httpserver;

#define PERM_CREATE_MESSAGES(p1)			((p1) & 0x3)
#define PERM_DELETE_MESSAGES(p1)			(((p1) >> 2) & 0x3)
#define PERM_CHANGE_SERVER(p1)				(((p1) >> 4) & 0x3)
#define PERM_KICK_MEMBERS(p1)				(((p1) >> 6) & 0x3)
#define PERM_BAN_MEMBERS(p1)				(((p1) >> 8) & 0x3)
#define PERM_MANAGE_CHANNELS(p1)			(((p1) >> 10) & 0x3)
#define PERM_MANAGE_INVITES(p1)				(((p1) >> 12) & 0x3)
#define PERM_SPEAK_IN_VC(p1)				(((p1) >> 14) & 0x3)


/* Don't forget to tx.commit(), utils do not commit automatically to avoid closing a transaction. */

class role_utils
{
public:
	static std::shared_ptr<http_response> parse_server_role_id(const http_request&, int role_id, pqxx::work&, int& server_user_id);
	// Checks if server has the role
	static std::shared_ptr<http_response> check_server_role(int role_id, int server_id, pqxx::work&);

	static int find_head_role(pqxx::work&, int server_id);
	static int find_lowest_role(pqxx::work&, int server_id);

	static std::vector<pqxx::row> get_role_list(pqxx::work&, int server_id);

	// -1 = next_role_id does not exist
	static int insert_role(pqxx::work&, int server_id,
					int next_role_id, std::string name, int color); // insert at the head if next_role_id = -1
	static int create_default_role_if_absent(pqxx::work&, int server_id);

	// moves roles to be before next_role_id, or at the head if next_role_id = -1
	static void move_role(pqxx::work&, int server_id,
					int role_id, int next_role_id);

	/* JSON */
	static nlohmann::json role_json_from_row(const pqxx::row r);
	static nlohmann::json role_json_from_row(const pqxx::row&& r);
};

#endif
