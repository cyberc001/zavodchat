#include "resource/role_utils.h"

int role_utils::find_lowest_role(pqxx::work& tx, int server_id)
{
	pqxx::result r = tx.exec("SELECT role_id FROM roles WHERE server_id = $1 AND prev_role_id = -1", pqxx::params(server_id));
	if(!r.size())
		return -1;
	return r[0]["role_id"].as<int>();
}

int role_utils::insert_role(pqxx::work& tx, int server_id,
					int next_role_id, std::string name, int color)
{
	pqxx::result r = tx.exec("INSERT INTO roles(prev_role_id, server_id, name, color, perms1) VALUES(-1, $1, $2, $3, $4) RETURNING role_id", pqxx::params(server_id, name, color, 0b0110101010101001));
	int role_id = r[0]["role_id"].as<int>();

	int next_prev_role_id = -1;
	if(next_role_id != -1){
		r = tx.exec("SELECT prev_role_id FROM roles WHERE role_id = $1", pqxx::params(next_role_id));
		if(r.size())
			next_prev_role_id = r[0]["prev_role_id"].as<int>();
		tx.exec("UPDATE roles SET prev_role_id = $1 WHERE role_id = $2", pqxx::params(role_id, next_role_id));
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
