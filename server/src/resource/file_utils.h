#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include "resource/utils.h"
#include <httpserver.hpp>
#include <filesystem>

class file_utils
{
public:
	/* CONFIG PARAMETERS */
	static std::string 	// avatars, indefinite storage but 1 image per user/server
				user_avatar_storage_path,
				server_avatar_storage_path,
				// files, limited space per user
				file_storage_path;
	static size_t file_storage_size;

	// Return an error if the file is bigger than the file size limit, cannot be parsed, or is not an image
	static std::shared_ptr<http_response> parse_user_avatar(const http_request&, std::string arg_name,
								int user_id, std::string& out_fname);
	static std::shared_ptr<http_response> parse_server_avatar(const http_request&, std::string arg_name,
								int server_id, std::string& out_fname);

	static std::string generate_fname(size_t sz = 32);
	static void save_file(const std::string_view& fraw, std::string fpath);

	static void fs_add_busy(pqxx::work& tx, int user_id, size_t bytes);
	static void fs_sub_busy(pqxx::work& tx, int user_id, size_t bytes);

	// Returns the size of the file deleted
	static size_t delete_oldest_file(pqxx::work& tx, int user_id);
	// If there isn't enough space:
	// first tries to roll over overflowing user storages;
	// then rolls over the user's files
	static std::shared_ptr<http_response> fs_make_space(const http_request& res, pqxx::work& tx, int user_id, size_t bytes);

private:
	static std::string get_image_ext(const std::string_view& fraw);
};

#endif
