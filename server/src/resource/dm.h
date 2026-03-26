#ifndef RESOURCE_DM_H
#define RESOURCE_DM_H

#include "socket/vc_server.h"
#include <resource/base.h>

class dm_resource : public base_resource
{
public:
	dm_resource(webserver& ws, db_connection_pool& pool, const config& cfg,
			socket_vc_server& vcserv);

	std::shared_ptr<http_response> render_GET(const http_request&);
private:
	socket_vc_server& vcserv;
};

class dm_id_resource : public base_resource
{
public:
	dm_id_resource(webserver& ws, db_connection_pool& pool, const config& cfg);

	std::shared_ptr<http_response> render_POST(const http_request&);
};

#endif
