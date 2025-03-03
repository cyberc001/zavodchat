#ifndef RESOURCE_FILE_H
#define RESOURCE_FILE_H

#include "db/conn_pool.h"

#include <httpserver.hpp>
using namespace httpserver;

// generic http resource that allows read-only file access
class file_resource : public http_resource
{
public:
	file_resource(std::filesystem::path storage_path);

	virtual std::shared_ptr<http_response> render_GET(const http_request&); // returns args[fname] binary data if present
protected:
	std::filesystem::path storage_path;
};

class server_file_resource : public http_resource // doesn't accept file_name argument
{
public:
	server_file_resource(db_connection_pool& pool, std::filesystem::path storage_path);

	std::shared_ptr<http_response> render_PUT(const http_request&); // uploads a file in flat_args[file] with extension from args[ext]

	unsigned max_tmp_files_per_user = 15;
	size_t max_ext_size = 10; // not included in config
private:
	std::filesystem::path storage_path;
	db_connection_pool& pool;
};
// http resource that also allows uploading files, temporary until they get attached to a message.
// forbids access to files from servers user is not a member of.
class server_file_id_resource : public file_resource
{
public:
	server_file_id_resource(db_connection_pool& pool, std::filesystem::path storage_path);

	std::shared_ptr<http_response> render_GET(const http_request&);
	std::shared_ptr<http_response> render_DELETE(const http_request&); // deletes a temporary file
private:
	db_connection_pool& pool;
};

#endif
