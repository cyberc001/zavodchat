#ifndef ROLE_UTILS_H
#define ROLE_UTILS_H

#include <pqxx/pqxx>

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
	static int find_lowest_role(pqxx::work&, int server_id);

	// -1 = next_role_id does not exist
	static int insert_role(pqxx::work&, int server_id,
					int next_role_id, std::string name, int color);

	static int create_default_role_if_absent(pqxx::work&, int server_id);
};

#endif
