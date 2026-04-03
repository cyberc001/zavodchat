#ifndef NOTIFICATIONS_RESOURCE_H
#define NOTIFICATIONS_RESOURCE_H

#include <resource/base.h>

class notifications_resource : public base_resource
{
public:
	notifications_resource(webserver& ws, db_connection_pool& pool, const config& cfg);

	std::shared_ptr<http_response> render_GET(const http_request&);
};

class notification_channel_resource : public base_resource
{
public:
	notification_channel_resource(webserver& ws, db_connection_pool& pool, const config& cfg);

	std::shared_ptr<http_response> render_DELETE(const http_request&);
};

#endif
