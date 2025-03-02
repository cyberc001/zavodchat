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

	std::shared_ptr<http_response> render_GET(const http_request&); // returns args[fname] binary data if present
private:
	std::filesystem::path storage_path;
};

#endif
