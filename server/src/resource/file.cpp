#include "resource/file.h"
#include "resource/utils.h"
#include "resource/file_utils.h"

file_resource::file_resource(std::filesystem::path storage_path): storage_path{storage_path}
{
	disallow_all();
	set_allowing("GET", true);
}

std::shared_ptr<http_response> file_resource::render_GET(const http_request& req)
{
	std::string_view fname = req.get_arg("fname");
	auto res = create_response::file(storage_path / fname);
	return res;
}

server_file_resource::server_file_resource(db_connection_pool& pool, std::filesystem::path storage_path): pool{pool}, storage_path{storage_path}
{
	disallow_all();
	set_allowing("PUT", true);
}
std::shared_ptr<http_response> server_file_resource::render_PUT(const http_request& req)
{
	int user_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_server_id(req, tx, user_id, server_id);
	if(err) return err;

	pqxx::result r;
	r = tx.exec("SELECT user_id FROM tmp_files WHERE user_id = $1", pqxx::params(user_id));
	if(r.size() >= max_tmp_files_per_user)
		return create_response::string("Too many temporary files per user", 403);

	std::string_view ext = req.get_arg("ext");
	if(!ext.size())
		return create_response::string("Empty file extension", 400);
	if(ext.size() > max_ext_size)
		return create_response::string("File extension is longer than " + std::to_string(max_ext_size), 400);
	std::string_view fraw = req.get_arg_flat("file");
	if(!fraw.size())
		return create_response::string("Empty file", 400);

	r = tx.exec("INSERT INTO tmp_files(filename, user_id) VALUES(gen_random_uuid(), $1) RETURNING filename", pqxx::params(user_id));
	tx.commit();
	std::string fname = r[0]["filename"].as<std::string>() + "." + std::string(ext);
	std::filesystem::create_directories(storage_path / std::to_string(server_id));
	file_utils::save_file(fraw, storage_path / std::to_string(server_id) / fname);
	return create_response::string(fname, 200);
}


server_file_id_resource::server_file_id_resource(db_connection_pool& pool, std::filesystem::path storage_path): file_resource(storage_path), pool{pool}
{
	set_allowing("DELETE", true);
}

std::shared_ptr<http_response> server_file_id_resource::render_GET(const http_request& req)
{
	int user_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_server_id(req, tx, user_id, server_id);
	if(err) return err;

	std::string_view fname = req.get_arg("fname");
	return create_response::file(storage_path / std::to_string(server_id) / fname);
}
std::shared_ptr<http_response> server_file_id_resource::render_DELETE(const http_request& req)
{
	int user_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_server_id(req, tx, user_id, server_id);
	if(err) return err;

	std::string_view fname = req.get_arg("fname");
	fname = fname.substr(0, fname.find_last_of('.'));

	pqxx::result r;
	r = tx.exec("SELECT user_id FROM tmp_files WHERE user_id = $1 AND filename = $2", pqxx::params(user_id, fname));
	if(!r.size())
		return create_response::string("File does not exist", 404);
	tx.exec("DELETE FROM tmp_files WHERE filename = $1", pqxx::params(fname));
	tx.commit();

	return create_response::string("Deleted", 200);
}
