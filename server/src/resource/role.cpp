#include "resource/role.h"

#include "resource/utils.h"
#include "resource/role_utils.h"

#include <iostream>

server_roles_resource::server_roles_resource(db_connection_pool& pool, socket_main_server& sserv) : base_resource(), pool{pool}, sserv{sserv}
{
	set_allowing("GET", true);
	set_allowing("PUT", true);
}

std::shared_ptr<http_response> server_roles_resource::render_GET(const http_request& req)
{
	base_resource::render_GET(req);

	int user_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_server_id(req, tx, user_id, server_id);
	if(err) return err;

	nlohmann::json res = nlohmann::json::array();
	std::vector<pqxx::row> r = role_utils::get_role_list(tx, server_id);
	for(size_t i = 0; i < r.size(); ++i)
		res += role_utils::role_json_from_row(r[i]);

	return create_response::string(req, res.dump(), 200);
}

int server_roles_resource::get_next_id(const std::vector<nlohmann::json>& list, int i)
{
	for(; i > -1; --i)
		if(list[i]["id"] > -1)
			return list[i]["id"];
	return -1;
}

std::shared_ptr<http_response> server_roles_resource::render_PUT(const http_request& req)
{
	base_resource::render_PUT(req);

	int user_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_server_id(req, tx, user_id, server_id);
	if(err) return err;

	err = role_utils::check_permission1(req, tx, server_id, user_id, PERM1_MANAGE_ROLES);
	if(err) return err;

	nlohmann::json list;
	err = resource_utils::json_from_content(req, list);
	if(err) return err;

	// Validate array
	if(!list.is_array())
		return create_response::string(req, "Content should be a JSON array", 400);
	if(list.size() >= max_per_server)
		return create_response::string(req, "Server has more than " + std::to_string(max_per_server) + " roles", 403);
	for(nlohmann::json::iterator i = list.begin(); i != list.end(); ++i){
		err = role_utils::check_role_json(req, *i);
		if(err) return err;
	}

	std::vector<pqxx::row> r = role_utils::get_role_list(tx, server_id);	
	std::vector<nlohmann::json> roles;
	for(size_t i = 0; i < r.size(); ++i){
		nlohmann::json j = role_utils::role_json_from_row(r[i]);
		roles.push_back(j);
	}

	// Remove roles
	for(size_t j = 0; j < roles.size(); ++j){
		size_t i; for(i = 0; i < list.size(); ++i)
			if(list[i]["id"] == roles[j]["id"])
				break;
		if(i == list.size()){
			err = role_utils::check_role_lower_than_user(req, tx, server_id, user_id, roles[j]["id"]);
			if(err) return err;
			err = role_utils::check_role_not_default(req, tx, server_id, roles[j]["id"]);
			if(err) return err;

			role_utils::delete_role(tx, server_id, roles[j]["id"]);
			roles.erase(roles.begin() + j--);
		}
	}
	for(size_t i = 0; i < list.size(); ++i){
		// Try to find an existing role with the same ID
		size_t j; for(j = 0; j < roles.size(); ++j)
			if(list[i]["id"] == roles[j]["id"])
				break;

		if(j != roles.size()){
			int role_next_id = get_next_id(roles, j - 1),
			    list_role_next_id = get_next_id(list, i - 1);
			bool ch_content = list[i] != roles[j], ch_pos = role_next_id != list_role_next_id;
			if(!ch_content && !ch_pos)
				continue;

			// check that user can modify this role
			err = role_utils::check_role_lower_than_user(req, tx, server_id, user_id, list[i]["id"]);
			if(err) return err;

			if(ch_content){
				roles[j] = list[i];

				int id = list[i]["id"].get<int>();
				std::string name = list[i]["name"].get<std::string>();

				unsigned long long perms1 = list[i]["perms1"].get<unsigned long long>();
				if(role_utils::find_default_role(tx, list[i]["id"]) == list[i]["id"])
					err = role_utils::check_default_validity_perms1(req, perms1);
				else
					err = role_utils::check_validity_perms1(req, perms1);
				if(err) return err;
		
				int color;
				err = resource_utils::string_to_color(req, list[i]["color"].get<std::string>(), color);
				if(err) return err;

				try{
					tx.exec("UPDATE roles SET name = $2, color = $3, perms1 = $4 WHERE role_id = $1",
							pqxx::params(id, name, color, perms1));
				} catch(pqxx::data_exception& e){
					return create_response::string(req, "Role name is too long", 400);
				}
			}

			if(ch_pos){
				// dont allow to move default role or to move below default role
				err = role_utils::check_role_not_default(req, tx, server_id, list[i]["id"]);
				if(err) return err;
				err = role_utils::check_role_not_default(req, tx, server_id, list_role_next_id);
				if(err) return err;
				// check that user is not moving this role above his
				err = role_utils::check_role_lower_than_user(req, tx, server_id, user_id, list_role_next_id, true);
				if(err) return err;

				nlohmann::json rol = roles[j];

				roles.erase(roles.begin() + j);
				if(list_role_next_id == -1)
					roles.insert(roles.begin(), rol);
				else{
					size_t k; for(k = 0; k < roles.size(); ++k)
						if(roles[k]["id"] == list_role_next_id)
							break;
					roles.insert(roles.begin() + k + 1, rol);
				}
				
				role_utils::move_role(tx, server_id, list[i]["id"], list_role_next_id);
			}
		}
	}
	// Add new roles
	for(int i = list.size() - 1; i > -1; --i)
		if(list[i]["id"] < 0){
			int list_role_next_id = get_next_id(list, i - 1);

			// Check that new role is not inserted above higher role
			err = role_utils::check_role_lower_than_user(req, tx, server_id, user_id, list_role_next_id, true);
			if(err) return err;

			std::string name = list[i]["name"].get<std::string>();
			
			unsigned long long perms1 = list[i]["perms1"].get<unsigned long long>();
			err = role_utils::check_validity_perms1(req, perms1);
			if(err) return err;
		
			int color;
			err = resource_utils::string_to_color(req, list[i]["color"].get<std::string>(), color);
			if(err) return err;

			int inserted_id = role_utils::insert_role(tx, server_id, list_role_next_id, name, color, perms1);
			if(inserted_id == -1)
				std::cerr << "INTERNAL ERROR: next_role_id " << list_role_next_id << " does not exist" << std::endl;
			if(inserted_id == -2)
				return create_response::string(req, "Role name is too long", 400);
		}

	socket_event ev;
	r = role_utils::get_role_list(tx, server_id);
	ev.data["roles"] = nlohmann::json::array();
	for(size_t i = 0; i < r.size(); ++i)
		ev.data["roles"] += role_utils::role_json_from_row(r[i]);
	resource_utils::json_set_ids(ev.data, server_id);
	ev.name = "roles_updated";
	sserv.send_to_server(server_id, tx, ev);

	tx.commit();
	return create_response::string(req, "", 200);
}
