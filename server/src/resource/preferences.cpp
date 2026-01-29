#include "preferences.h"
#include <resource/utils.h>
#include <unordered_set>

preferences_resource::preferences_resource(db_connection_pool& pool): base_resource(), pool{pool}
{
	set_allowing("GET", true);
	set_allowing("PUT", true);
	set_allowing("POST", true);
}

std::shared_ptr<http_response> preferences_resource::render_GET(const http_request& req)
{
	base_resource::render_GET(req);

	db_connection conn = pool.hold();
	pqxx::work tx{*conn};

	int user_id;
	auto err = resource_utils::parse_session_token(req, tx, user_id);
	if(err) return err;

	pqxx::result r;
	r = tx.exec("SELECT key, value FROM user_preferences WHERE user_id = $1", pqxx::params(user_id));
	nlohmann::json res = nlohmann::json::object();
	for(size_t i = 0; i < r.size(); ++i)
		res[r[i]["key"].as<std::string>()] = r[i]["value"].as<std::string>();
	return create_response::string(req, res.dump(), 200);
}

std::shared_ptr<http_response> preferences_resource::render_PUT(const http_request& req)
{
	base_resource::render_PUT(req);

	db_connection conn = pool.hold();
	pqxx::work tx{*conn};

	int user_id;
	auto err = resource_utils::parse_session_token(req, tx, user_id);
	if(err) return err;

	nlohmann::json new_prefs;
	err = resource_utils::json_from_content(req, new_prefs);
	if(err) return err;

	pqxx::result r;
	r = tx.exec("SELECT key, value FROM user_preferences WHERE user_id = $1", pqxx::params(user_id));
	std::unordered_set<std::string> keys;
	for(size_t i = 0; i < r.size(); ++i)
		keys.insert(r[i]["key"].as<std::string>());

	for(nlohmann::json::iterator it = new_prefs.begin(); it != new_prefs.end(); ++it)
		if(keys.count(it.key()))
			tx.exec("UPDATE user_preferences SET value = $3 WHERE user_id = $1 AND key = $2", pqxx::params(user_id, it.key(), json_value_string(it.value())));
		else {
			if(keys.size() >= keys_per_user)
				return create_response::string(req, "Amount of pairs is bigger than " + std::to_string(keys_per_user), 400);
			keys.insert(it.key());
			tx.exec("INSERT INTO user_preferences(user_id, key, value) VALUES($1, $2, $3)", pqxx::params(user_id, it.key(), json_value_string(it.value())));
		}
	tx.commit();

	return create_response::string(req, "Changed", 200);
}

std::shared_ptr<http_response> preferences_resource::render_POST(const http_request& req)
{
	base_resource::render_POST(req);

	db_connection conn = pool.hold();
	pqxx::work tx{*conn};

	int user_id;
	auto err = resource_utils::parse_session_token(req, tx, user_id);
	if(err) return err;

	nlohmann::json new_prefs;
	err = resource_utils::json_from_content(req, new_prefs);
	if(err) return err;
	if(new_prefs.size() > keys_per_user)
		return create_response::string(req, "Amount of pairs is bigger than " + std::to_string(keys_per_user), 400);

	tx.exec("DELETE FROM user_preferences WHERE user_id = $1", pqxx::params(user_id));
	try {
		for(nlohmann::json::iterator it = new_prefs.begin(); it != new_prefs.end(); ++it)
			tx.exec("INSERT INTO user_preferences(user_id, key, value) VALUES($1, $2, $3)", pqxx::params(user_id, it.key(), json_value_string(it.value())));
	} catch(pqxx::data_exception& e) {
		return create_response::string(req, "A key or a value is too long", 400);
	}
	tx.commit();

	return create_response::string(req, "Replaced", 200);
}

std::string preferences_resource::json_value_string(const nlohmann::json& j)
{
	if(j.is_string())
		return j.get<std::string>();
	return j.dump();
}
