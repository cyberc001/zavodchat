#ifndef RESOURCE_CHANNEL_MESSAGES_H
#define RESOURCE_CHANNEL_MESSAGES_H

#include "db/conn_pool.h"
#include "socket/main_server.h"
#include <resource/base.h>

enum {
	MESSAGE_ATTACHMENT_LINK = 0,
	MESSAGE_ATTACHMENT_FILE,
	MESSAGE_ATTACHMENT_IMAGE
};

class channel_messages_resource : public base_resource
{
public:
	channel_messages_resource(webserver& ws, db_connection_pool& pool, const config& cfg,
					socket_main_server& sserv);

	std::shared_ptr<http_response> render_GET(const http_request&);
	std::shared_ptr<http_response> render_POST(const http_request&);

	static std::shared_ptr<http_response> parse_attachment(const http_request&, nlohmann::json&,
								std::string& content, unsigned& att_type);
	static bool is_valid_attachment_type(unsigned type);
private:
	socket_main_server& sserv;
};

class channel_messages_search_resource : public base_resource
{
public:
	channel_messages_search_resource(webserver& ws, db_connection_pool& pool, const config& cfg,
						socket_main_server& sserv);

	std::shared_ptr<http_response> render_POST(const http_request&);
private:
	socket_main_server& sserv;
};

class message_resource : public base_resource
{
public:
	message_resource(webserver& ws, db_connection_pool& pool, const config& cfg,
						socket_main_server& sserv);

	std::shared_ptr<http_response> render_GET(const http_request&);
	std::shared_ptr<http_response> render_PUT(const http_request&);
	std::shared_ptr<http_response> render_DELETE(const http_request&);
private:
	socket_main_server& sserv;
};

#endif
