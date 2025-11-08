#include "resource/file.h"
#include "resource/utils.h"
#include "resource/file_utils.h"

file_resource::file_resource(std::filesystem::path storage_path): base_resource(), storage_path{storage_path}
{
	set_allowing("GET", true);
}

std::shared_ptr<http_response> file_resource::render_GET(const http_request& req)
{
	std::string_view fname = req.get_arg("fname");
	auto res = create_response::file(req, storage_path / fname);
	return res;
}

server_file_put_resource::server_file_put_resource(db_connection_pool& pool, std::filesystem::path storage_path): base_resource(), pool{pool}, storage_path{storage_path}
{
	set_allowing("PUT", true);
}
std::shared_ptr<http_response> server_file_put_resource::render_PUT(const http_request& req)
{
	int user_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_session_token(req, tx, user_id);
	if(err) return err;

	std::string_view ext = req.get_arg("ext");
	if(!ext.size())
		return create_response::string(req, "Empty file extension", 400);
	if(ext.size() > max_ext_size)
		return create_response::string(req, "File extension is longer than " + std::to_string(max_ext_size), 400);
	std::string_view fraw = req.get_arg_flat("file");
	if(!fraw.size())
		return create_response::string(req, "Empty file", 400);

	err = file_utils::fs_make_space(req, tx, user_id, fraw.size());
	if(err) return err;

	std::string fname = file_utils::generate_fname() + "." + std::string(ext);
	std::filesystem::create_directories(storage_path / std::to_string(user_id));
	file_utils::save_file(fraw, storage_path / std::to_string(user_id) / fname);

	file_utils::fs_add_busy(tx, user_id, fraw.size());
	return create_response::string(req, fname, 200);
}


server_file_manage_resource::server_file_manage_resource(db_connection_pool& pool, std::filesystem::path storage_path): base_resource(), pool{pool}, storage_path{storage_path}
{
	set_allowing("DELETE", true);
}

std::shared_ptr<http_response> server_file_manage_resource::render_DELETE(const http_request& req)
{
	int user_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_session_token(req, tx, user_id);
	if(err) return err;

	std::string_view fname = req.get_arg("fname");
	//fname = fname.substr(0, fname.find_last_of('.'));

	std::filesystem::path fpath = storage_path / std::to_string(user_id) / fname;
	if(!std::filesystem::exists(fpath))
		return create_response::string(req, "File does not exist", 404);

	size_t fsize = std::filesystem::file_size(fpath);
	std::filesystem::remove(fpath);
	file_utils::fs_sub_busy(tx, user_id, fsize);
	return create_response::string(req, "Deleted", 200);
}

server_user_file_resource::server_user_file_resource(db_connection_pool& pool, std::filesystem::path storage_path): base_resource(), pool{pool}, storage_path{storage_path}
{
	set_allowing("GET", true);
}
std::shared_ptr<http_response> server_user_file_resource::render_GET(const http_request& req)
{
	int user_id, author_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_session_token(req, tx, user_id);
	if(err) return err;
	err = resource_utils::parse_index(req, "user_id", author_id);
	if(err) return err;

	std::string_view fname = req.get_arg("fname");
	return create_response::file(req, storage_path / std::to_string(author_id) / fname);
}

