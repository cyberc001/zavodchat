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
	channel_messages_resource(db_connection_pool& pool, socket_main_server& sserv);

	std::shared_ptr<http_response> render_GET(const http_request&);
	std::shared_ptr<http_response> render_POST(const http_request&);

	/* CONFIG PARAMETERS */
	unsigned max_get_count = 50;
	unsigned max_attachments = 10;

	static std::shared_ptr<http_response> parse_attachment(const http_request&, nlohmann::json&,
								std::string& content, unsigned& att_type, int file_user_id);
	static bool is_valid_attachment_type(unsigned type);
private:
	db_connection_pool& pool;
	socket_main_server& sserv;
};

class channel_messages_search_resource : public base_resource
{
public:
	channel_messages_search_resource(db_connection_pool& pool);

	std::shared_ptr<http_response> render_POST(const http_request&);

	/* CONFIG PARAMETERS */
	unsigned max_get_count = 50;
private:
	db_connection_pool& pool;
};

class message_resource : public base_resource
{
public:
	message_resource(db_connection_pool& pool, socket_main_server& sserv);

	std::shared_ptr<http_response> render_GET(const http_request&);
	std::shared_ptr<http_response> render_PUT(const http_request&);
	std::shared_ptr<http_response> render_DELETE(const http_request&);

	unsigned max_attachments = 10;
private:
	db_connection_pool& pool;
	socket_main_server& sserv;

	bool is_valid_attachment_change_type(unsigned type);
};

#endif
