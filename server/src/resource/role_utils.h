#ifndef ROLE_UTILS_H
#define ROLE_UTILS_H

#include <pqxx/pqxx>
#include <nlohmann/json.hpp>
#include <vector>
#include <httpserver.hpp>
using namespace httpserver;

#define PERM1_CREATE_MESSAGES			0
#define PERM1_DELETE_MESSAGES			1
#define PERM1_CHANGE_SERVER			2
#define PERM1_KICK_MEMBERS			3
#define PERM1_BAN_MEMBERS			4
#define PERM1_MANAGE_CHANNELS			5
#define PERM1_MANAGE_INVITES			6
#define PERM1_SPEAK_IN_VC			7
#define PERM1_MANAGE_ROLES			8

#define PERM1_COUNT				9
#define PERM1_DEFAULT				0b100110101010101001


/* Don't forget to tx.commit(), utils do not commit automatically to avoid closing a transaction. */

class role_utils
{
public:
	static std::shared_ptr<http_response> parse_server_role_id(const http_request&, int role_id, pqxx::work&, int& server_user_id);
	// Checks if server has the role
	static std::shared_ptr<http_response> check_server_role(int role_id, int server_id, pqxx::work&);

	static int find_head_role(pqxx::work&, int server_id);
	static int find_lowest_role(pqxx::work&, int server_id);
	static int find_default_role(pqxx::work&, int server_id);

	static std::vector<pqxx::row> get_role_list(pqxx::work&, int server_id);
	static bool is_role_higher(pqxx::work&, int server_id, int role_id, int other_role_id);

	static std::shared_ptr<http_response> check_role_not_default(pqxx::work&, int server_id, int role_id);
	// returns OK (nullptr) if the user is the owner of the server
	static std::shared_ptr<http_response> check_role_lower_than_user(pqxx::work&, int server_id, int user_id, int role_id, bool can_be_equal = false);
	// returns OK (nullptr) if the user is the owner of the server
	static std::shared_ptr<http_response> check_permission1(pqxx::work&, int server_id, int user_id, int perm);
	static std::shared_ptr<http_response> check_validity_perms1(long long perms1);

	// -1 = next_role_id does not exist
	static int insert_role(pqxx::work&, int server_id,
					int next_role_id, std::string name, int color, long long perms1 = PERM1_DEFAULT); // insert at the head if next_role_id = -1
	static int create_default_role_if_absent(pqxx::work&, int server_id);

	// moves roles to be before next_role_id, or at the head if next_role_id = -1
	static void move_role(pqxx::work&, int server_id,
					int role_id, int next_role_id);

	/* JSON */
	static nlohmann::json role_json_from_row(const pqxx::row r);
	static nlohmann::json role_json_from_row(const pqxx::row&& r);
};

#endif
