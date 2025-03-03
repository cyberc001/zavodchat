#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include "resource/utils.h"
#include <httpserver.hpp>
#include <filesystem>

class file_utils
{
public:
	static std::string 	// avatars, indefinite storage but 1 image per user/server
				user_avatar_storage_path,
				server_avatar_storage_path;

	// Return an error if the file is bigger than the file size limit, cannot be parsed, or is not an image
	static std::shared_ptr<http_response> parse_user_avatar(const http_request&, std::string arg_name,
								int user_id, std::string& out_fname);
	static std::shared_ptr<http_response> parse_server_avatar(const http_request&, std::string arg_name,
								int server_id, std::string& out_fname);

	static void save_file(const std::string_view& fraw, std::string fpath);

private:
	static std::string get_image_ext(const std::string_view& fraw);
};

#endif
