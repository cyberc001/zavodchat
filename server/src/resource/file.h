#ifndef RESOURCE_FILE_H
#define RESOURCE_FILE_H

#include "db/conn_pool.h"
#include <resource/base.h>

// generic http resource that allows read-only file access
class file_resource : public base_resource
{
public:
	file_resource(std::filesystem::path storage_path);

	virtual std::shared_ptr<http_response> render_GET(const http_request&); // returns args[fname] binary data if present
private:
	std::filesystem::path storage_path;
};

// http resources that allows uploading files, with each user getting an equal share of set storage space.
// uses rollover when storage space overflows.
// forbids access to files from servers user is not a member of.
class server_file_put_resource : public base_resource // no args
{
public:
	server_file_put_resource(db_connection_pool& pool, std::filesystem::path storage_path);

	std::shared_ptr<http_response> render_PUT(const http_request&); // uploads a file in flat_args[file] with extension from args[ext]

	size_t max_ext_size = 10; // not included in config
private:
	std::filesystem::path storage_path;
	db_connection_pool& pool;
};
class server_file_manage_resource : public base_resource // {fname}
{
public:
	server_file_manage_resource(db_connection_pool& pool, std::filesystem::path storage_path);

	std::shared_ptr<http_response> render_DELETE(const http_request&);

	size_t max_ext_size = 10; // not included in config
private:
	std::filesystem::path storage_path;
	db_connection_pool& pool;
};

class server_user_file_resource : public base_resource // {user_id} {fname}
{
public:
	server_user_file_resource(db_connection_pool& pool, std::filesystem::path storage_path);

	std::shared_ptr<http_response> render_GET(const http_request&);
private:
	std::filesystem::path storage_path;
	db_connection_pool& pool;
};

#endif
