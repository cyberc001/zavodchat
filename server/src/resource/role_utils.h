#ifndef ROLE_UTILS_H
#define ROLE_UTILS_H

#include <pqxx/pqxx>
#include <nlohmann/json.hpp>
#include <vector>
#include <httpserver.hpp>
using namespace httpserver;

#define GET_PERM_BIT(perms, bit) ((perms) >> ((bit) * 2) & 0x3)


#define PERM1_CREATE_MESSAGES			0
#define PERM1_DELETE_MESSAGES			1
#define PERM1_CHANGE_SERVER			2
#define PERM1_KICK_MEMBERS			3
#define PERM1_BAN_MEMBERS			4
#define PERM1_MANAGE_CHANNELS			5
#define PERM1_MANAGE_INVITES			6
#define PERM1_JOIN_VC				7
#define PERM1_MANAGE_ROLES			8
#define PERM1_MANAGE_VC				9

#define PERM1_COUNT				10
#define PERM1_DEFAULT				0b10100110101010101001

/* Don't forget to tx.commit(), utils do not commit automatically to avoid closing a transaction. */

class role_utils
{
public:
	struct perms_info {
		perms_info(std::string column, int count): column(column), count(count){}

		std::string column;
		int count;
	};
	static perms_info perms1;	

	static std::shared_ptr<http_response> parse_server_role_id(const http_request&, int role_id, pqxx::work&, int& server_user_id);
	// Checks if server has the role
	static std::shared_ptr<http_response> check_server_role(const http_request&, int role_id, int server_id, pqxx::work&);

	static int find_head_role(pqxx::work&, int server_id);
	static int find_lowest_role(pqxx::work&, int server_id);
	static int find_default_role(pqxx::work&, int server_id);

	static std::vector<pqxx::row> get_role_list(pqxx::work&, int server_id);
	static std::vector<pqxx::row> get_user_role_list(pqxx::work&, int server_id, int user_id);
	static bool is_role_higher(pqxx::work&, int server_id, int role_id, int other_role_id);

	static std::shared_ptr<http_response> check_role_not_default(const http_request&, pqxx::work&, int server_id, int role_id);
	// returns OK (nullptr) if the user is the owner of the server

	static std::shared_ptr<http_response> check_role_lower_than_user(const http_request&, pqxx::work&, int server_id, int user_id, int role_id, bool can_be_equal = false);
	// returns OK (nullptr) if the user is the owner of the server
	static std::shared_ptr<http_response> check_user_lower_than_other(const http_request&, pqxx::work&, int server_id, int user_id, int lower_user_id, bool can_be_equal = false);

	// Compiles permission bits from the whole role hierarchy
	// Doesn't terminate early like check_permission()
	static long long get_permission_bits(pqxx::work&,
						int server_id, int user_id,
						const role_utils::perms_info& perm);

	static bool check_permission(pqxx::work&,
					int server_id, int user_id,
				       	const role_utils::perms_info& perm, int bit,
					int channel_id = -1);
	static std::shared_ptr<http_response> check_permission(const http_request&, pqxx::work&,
								int server_id, int user_id,
								const role_utils::perms_info& perm, int bit,
								int channel_id = -1);

	// Checks:
	// - All permission bits in count < 0x3
	// - All permission bits are either 0x1 or 0x2 if check_default = true
	// - All permission bits out of count == 0x0
	// - User does not set bits = 0x1 for permissions they don't have themselves
	//   (ex. admin cannot manage servers, but tries to enable that bit for mods)
	static std::shared_ptr<http_response> check_permission_validity(const http_request&, pqxx::work&,
									const role_utils::perms_info& perm,
									long long prev_perms, long long perms,
									int server_id, int user_id,
									bool check_default = false);

	// -1 = next_role_id does not exist
	// -2 = name is too long
	static int insert_role(pqxx::work&, int server_id,
					int next_role_id, std::string name, int color, long long perms1 = PERM1_DEFAULT); // insert at the head if next_role_id = -1
	static int create_default_role_if_absent(pqxx::work&, int server_id);

	// moves roles to be before next_role_id, or at the head if next_role_id = -1
	static void move_role(pqxx::work&, int server_id,
					int role_id, int next_role_id);

	static void delete_role(pqxx::work&, int server_id, int role_id);

	/* JSON */
	static std::shared_ptr<http_response> check_role_json(const http_request&, const nlohmann::json&);
	static nlohmann::json role_json_from_row(const pqxx::row r);
	static nlohmann::json role_json_from_row(const pqxx::row&& r);
};

#endif
