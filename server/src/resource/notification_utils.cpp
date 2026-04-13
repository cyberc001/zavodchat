#include "resource/notification_utils.h"
#include "resource/utils.h"
#include <unordered_set>
#include <sstream>

mention::mention(mention_types type, int begin_i, int end_i, int id):
	type{type}, begin_i{begin_i}, end_i{end_i}, id{id}
{}

std::vector<std::vector<int>> mention::to_array(const std::vector<mention>& in)
{
	std::vector<std::vector<int>> out;
	for(auto i = in.begin(); i != in.end(); ++i)
		out.push_back(std::vector<int>{i->type, i->begin_i, i->end_i, i->id});
	return out;
}
std::string mention::join_ids(const std::vector<mention>& in, mention_types type)
{
	std::stringstream ss;
	bool first = true;
	for(auto i = in.begin(); i != in.end(); ++i)
		if(i->type == type){
			if(!first)
				ss << ",";
			first = false;
			ss << i->id;
		}
	return ss.str();
}

bool mention::type_has_id(mention_types type)
{
	return type == mention_types::USER || type == mention_types::ROLE;
}


void notification_utils::inc_notification(int server_id, int channel_id, int user_id, pqxx::work& tx)
{
	pqxx::params pr(channel_id, user_id);
	if(server_id > -1)
		pr.append(server_id);
	tx.exec("INSERT INTO notifications(" + std::string(server_id > -1 ? "server_id, " : "") + "channel_id, user_id) "
		"VALUES(" + std::string(server_id > -1 ? "$3, " : "") + "$1, $2) "
		"ON CONFLICT(channel_id, user_id) DO UPDATE "
		"SET notification_count = notifications.notification_count + 1",
		pr);
}
void notification_utils::inc_notification(int server_id, int channel_id, int user_id, const std::vector<int>& user_ids,
						db_connection& conn, pqxx::work& tx)
{
	conn.prepare("inc_notification",
		     "INSERT INTO notifications(" + std::string(server_id > -1 ? "server_id, " : "") + "channel_id, user_id) "
		     "VALUES(" + std::string(server_id > -1 ? "$3, " : "") + "$1, $2) "
		     "ON CONFLICT(channel_id, user_id) DO UPDATE "
		     "SET notification_count = notifications.notification_count + 1");

	if(server_id > -1){
		for(auto i = user_ids.begin(); i != user_ids.end(); ++i)
			if(*i != user_id)
				tx.exec(pqxx::prepped{"inc_notification"}, pqxx::params(channel_id, *i, server_id));
	} else {
		for(auto i = user_ids.begin(); i != user_ids.end(); ++i)
			if(*i != user_id)
				tx.exec(pqxx::prepped{"inc_notification"}, pqxx::params(channel_id, *i));
	}
}
void notification_utils::inc_notification(int server_id, int channel_id, int user_id, const std::vector<mention>& mentions,
						db_connection& conn, pqxx::work& tx)
{
	// Only send one notification per @e / @r<id>
	bool mentioned_everyone = false;
	std::unordered_set<int> mentioned_roles;

	for(auto i = mentions.begin(); i != mentions.end(); ++i)
		switch(i->type){
			case mention_types::USER:
				if(i->id != user_id)
					inc_notification(server_id, channel_id, i->id, tx);
				break;
			case mention_types::ROLE:
				if(!mentioned_roles.count(i->id)){
					mentioned_roles.insert(i->id);
					std::vector<int> user_ids = resource_utils::get_role_users(i->id, tx);
					inc_notification(server_id, channel_id, user_id, user_ids,
								conn, tx);
				}
				break;
			case mention_types::EVERYONE:
				if(!mentioned_everyone){
					mentioned_everyone = true;
					std::vector<int> user_ids = resource_utils::get_channel_users(channel_id, tx);
					inc_notification(server_id, channel_id, user_id, user_ids,
								conn, tx);
				}
				break;
		}
}
void notification_utils::ensure_notification(int server_id, int channel_id, int user_id, const std::vector<int>& user_ids,
						db_connection& conn, pqxx::work& tx)
{
	conn.prepare("ensure_notification",
		     "INSERT INTO notifications(" + std::string(server_id > -1 ? "server_id, " : "") + "channel_id, user_id, notification_count) "
		     "VALUES(" + std::string(server_id > -1 ? "$3, " : "") + "$1, $2, 0) "
		     "ON CONFLICT(channel_id, user_id) DO NOTHING");

	if(server_id > -1){
		for(auto i = user_ids.begin(); i != user_ids.end(); ++i)
			if(*i != user_id)
				tx.exec(pqxx::prepped{"ensure_notification"}, pqxx::params(channel_id, *i, server_id));
	} else {
		for(auto i = user_ids.begin(); i != user_ids.end(); ++i)
			if(*i != user_id)
				tx.exec(pqxx::prepped{"ensure_notification"}, pqxx::params(channel_id, *i));
	}
}


mention_types char_to_mention_type(char c)
{
	switch(c){
		case 'u': return mention_types::USER;
		case 'r': return mention_types::ROLE;
		case 'e': return mention_types::EVERYONE;
	}
	return mention_types::INVALID;
}

std::vector<mention> notification_utils::parse_mentions(const std::string& text, int server_id, int channel_id, pqxx::work& tx)
{
	std::vector<mention> parsed, out;

	if(!text.size())
		return out;

	int state = 0;
	int id_start; mention_types type;
	for(int i = 0; i < text.size() && parsed.size() < max_mentions; ++i)
		switch(state){
			case 0: // Seeking a mention
				if(text[i] == '@')
					state = 1;
				break;
			case 1: // Seeking mention type
				type = char_to_mention_type(text[i]);
				if(type != mention_types::INVALID){
					if(mention::type_has_id(type) &&
						i < text.size() - 1 && isdigit(text[i + 1])){
						id_start = i + 1;
						state = 2;
					} else if(!mention::type_has_id(type)){
						parsed.emplace_back(type, i - 1, i);
						state = 0;
					}
				} else
					state = 0;
				break;
			case 2: // Reading ID
				if(!isdigit(text[i]) || i == text.size() - 1){
					state = 0;
					i += (i == text.size() - 1); // next iteration won't happen, so for convenience i is getting modified to extend past string size
					int id = std::strtoul(text.substr(id_start, i - id_start).c_str(), nullptr, 10);
					parsed.emplace_back(type, id_start - 2, i - 1, id);
					i -= (i != text.size() - 1); // don't skip the next character after mention
				}
				break;
		}

	out.reserve(parsed.size());

	// Verify user IDs
	std::unordered_set<int> valid_user_ids;
	std::string user_ids = mention::join_ids(parsed, mention_types::USER);
	if(user_ids.size()){
		pqxx::result r = tx.exec("SELECT DISTINCT ON(user_id) user_id FROM user_x_server "
					 "WHERE server_id = $1 AND user_id IN (" + user_ids + ")",
					 pqxx::params(server_id));
		for(size_t i = 0; i < r.size(); ++i)
			valid_user_ids.insert(r[i]["user_id"].as<int>());
	}

	// Verify role IDs
	std::unordered_set<int> valid_role_ids;
	std::string role_ids = mention::join_ids(parsed, mention_types::ROLE);
	if(role_ids.size()){
		pqxx::result r = tx.exec("SELECT DISTINCT ON(role_id) role_id FROM roles "
					 "WHERE server_id = $1 AND role_id IN (" + role_ids + ")",
					 pqxx::params(server_id));
		for(size_t i = 0; i < r.size(); ++i)
			valid_role_ids.insert(r[i]["role_id"].as<int>());
	}

	// Compile all mentions
	for(auto i = parsed.begin(); i != parsed.end(); ++i)
		if(i->id < 0 ||
			(i->type == mention_types::USER && valid_user_ids.count(i->id)) ||
			(i->type == mention_types::ROLE && valid_role_ids.count(i->id)))
			out.push_back(*i);
	return out;
}
