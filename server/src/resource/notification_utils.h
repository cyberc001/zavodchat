#ifndef NOTIFICATION_UTILS_H
#define NOTIFICATION_UTILS_H

#include "db/conn_pool.h"
#include <vector>

enum mention_types
{
	INVALID,
	USER, ROLE, EVERYONE
};

struct mention
{
	mention(mention_types type, int begin_i, int end_i, int id = -1);

	static std::vector<std::vector<int>> to_array(const std::vector<mention>&);
	static std::string join_ids(const std::vector<mention>&, mention_types type);

	static bool type_has_id(mention_types);

	mention_types type;
	// character range [begin_i, end_i], includes @u
	int begin_i, end_i;
	int id; // optional, can be -1
};

class notification_utils
{
public:
	// Increments or sets notification_count to 1 if row does not exist.
	// server_id can be -1 for DMs.
	// When working with vectors, user_id blacklists this user from receiving a notification. user_id can be -1 to blacklist noone.
	static void inc_notification(int server_id, int channel_id, int user_id, pqxx::work&);
	static void inc_notification(int server_id, int channel_id, int user_id, const std::vector<int>& user_ids,
					db_connection&, pqxx::work&);
	static void inc_notification(int server_id, int channel_id, int user_id, const std::vector<mention>&,
					db_connection&, pqxx::work&);
	// If row does not exist, sets notification_count to 0 to indicate unread messages.
	static void ensure_notification(int server_id, int channel_id, int user_id, const std::vector<int>& user_ids,
					db_connection&, pqxx::work&);

	static std::vector<mention> parse_mentions(const std::string& text, int server_id, int channel_id, pqxx::work&);

	static const int max_mentions = 32;
};

#endif
