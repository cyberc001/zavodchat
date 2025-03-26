#include "resource/role_utils.h"
#include <unordered_map>
#include "resource/utils.h" 

std::shared_ptr<http_response> role_utils::parse_server_role_id(const http_request& req, int server_id, pqxx::work& tx, int& server_role_id)
{
	try{
		server_role_id = std::stoi(std::string(req.get_arg("server_role_id")));
	} catch(std::invalid_argument& e){
		return create_response::string("Invalid server role ID", 400);
	}
	pqxx::result r = tx.exec("SELECT role_id FROM roles WHERE role_id = $1 AND server_id = $2", pqxx::params(server_role_id, server_id));
	if(!r.size())
		return create_response::string("Role does not belong to the server", 404);
	return nullptr;
}
std::shared_ptr<http_response> role_utils::check_server_role(int role_id, int server_id, pqxx::work& tx)
{
	pqxx::result r = tx.exec("SELECT role_id FROM roles WHERE role_id = $1 AND server_id = $2", pqxx::params(role_id, server_id));
	if(!r.size())
		return create_response::string("Role does not belong to the server", 404);
	return nullptr;
}

int role_utils::find_head_role(pqxx::work& tx, int server_id)
{
	pqxx::result r = tx.exec("SELECT head_role_id FROM servers WHERE server_id = $1", pqxx::params(server_id));
	if(!r.size())
		return -1;
	return r[0]["head_role_id"].as<int>();
}
int role_utils::find_lowest_role(pqxx::work& tx, int server_id)
{
	pqxx::result r = tx.exec("SELECT role_id FROM roles WHERE server_id = $1 AND prev_role_id = -1", pqxx::params(server_id));
	if(!r.size())
		return -1;
	return r[0]["role_id"].as<int>();
}

std::vector<pqxx::row> role_utils::get_role_list(pqxx::work& tx, int server_id)
{
	int head = find_head_role(tx, server_id);
	pqxx::result r = tx.exec("SELECT role_id, prev_role_id, name, color, perms1 FROM roles WHERE server_id = $1", pqxx::params(server_id));
	std::unordered_map<int, pqxx::row> rm;
	for(size_t i = 0; i < r.size(); ++i)
		rm[r[i]["role_id"].as<int>()] = r[i];

	std::vector<pqxx::row> res;
	for(int cur = head; cur != -1; cur = rm[cur]["prev_role_id"].as<int>())
		res.push_back(rm[cur]);
	return res;
}


int role_utils::insert_role(pqxx::work& tx, int server_id,
					int next_role_id, std::string name, int color)
{
	pqxx::result r = tx.exec("INSERT INTO roles(prev_role_id, server_id, name, color, perms1) VALUES(-1, $1, $2, $3, $4) RETURNING role_id", pqxx::params(server_id, name, color, 0b0110101010101001));
	int role_id = r[0]["role_id"].as<int>();

	r = tx.exec("SELECT head_role_id FROM servers WHERE server_id = $1", pqxx::params(server_id));
	if(!r.size())
		return -1;
	int head_role_id = r[0]["head_role_id"].as<int>();
	if(head_role_id == -1)
		tx.exec("UPDATE servers SET head_role_id = $1 WHERE server_id = $2", pqxx::params(role_id, server_id));

	int next_prev_role_id = -1;
	if(next_role_id != -1){
		r = tx.exec("SELECT prev_role_id FROM roles WHERE role_id = $1", pqxx::params(next_role_id));
		if(r.size())
			next_prev_role_id = r[0]["prev_role_id"].as<int>();
		tx.exec("UPDATE roles SET prev_role_id = $1 WHERE role_id = $2", pqxx::params(role_id, next_role_id));
	} else {
		next_prev_role_id = head_role_id;
		if(head_role_id != -1) // duplicate request to set the head role
			r = tx.exec("UPDATE servers SET head_role_id = $1 WHERE server_id = $2", pqxx::params(role_id, server_id));
	}

	tx.exec("UPDATE roles SET prev_role_id = $1 WHERE role_id = $2", pqxx::params(next_prev_role_id, role_id));
	return role_id;
}
int role_utils::create_default_role_if_absent(pqxx::work& tx, int server_id)
{
	pqxx::result r = tx.exec("SELECT default_role_id FROM servers WHERE server_id = $1", pqxx::params(server_id));
	if(!r.size())
		return -1;
	if(r[0]["default_role_id"].as<int>() != -1)
		return -1;

	return insert_role(tx, server_id, find_lowest_role(tx, server_id), "default", 0x999999);
}

static void move_role(pqxx::work& tx, int server_id,
			int role_id, int next_role_id)
{
	pqxx::result r = tx.exec("SELECT prev_role_id FROM roles WHERE role_id = $1", pqxx::params(role_id));
	int old_prev_role_id = r[0]["prev_role_id"].as<int>();
	tx.exec("UPDATE roles SET prev_role_id = $1 WHERE prev_role_id = $2", pqxx::params(old_prev_role_id, role_id)); // update prev_role_id on role before role_id

	int new_prev_role_id = -1;
	if(next_role_id == -1){ // update head role_id
		r = tx.exec("SELECT head_role_id FROM servers WHERE server_id = $1", pqxx::params(server_id));
		new_prev_role_id = r[0]["head_role_id"].as<int>();
		tx.exec("UPDATE servers SET head_role_id = $1 WHERE server_id = $2", pqxx::params(role_id, server_id));
	} else { // update prev_role_id of next_role_id
		r = tx.exec("SELECT prev_role_id FROM roles WHERE role_id = $1", pqxx::params(next_role_id));
		new_prev_role_id = r[0]["prev_role_id"].as<int>();
		tx.exec("UPDATE roles SET prev_role_id = $1 WHERE role_id = $2", pqxx::params(role_id, next_role_id));
	}
	tx.exec("UPDATE roles SET prev_role_id = $1 WHERE role_id = $2", pqxx::params(new_prev_role_id, role_id));
}

/* JSON */
nlohmann::json role_utils::role_json_from_row(const pqxx::row r)
{
	nlohmann::json res = {{"id", r["role_id"].as<int>()},
				{"name", r["name"].as<std::string>()},
				{"color", r["color"].as<int>()},
				{"perms1", r["perms1"].as<int>()}};
	return res;
}
nlohmann::json role_utils::role_json_from_row(const pqxx::row&& r)
{
	nlohmann::json res = {{"id", r["role_id"].as<int>()},
				{"name", r["name"].as<std::string>()},
				{"color", r["color"].as<int>()},
				{"perms1", r["perms1"].as<int>()}};
	return res;
}
