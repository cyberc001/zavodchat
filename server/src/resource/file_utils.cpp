#include "file_utils.h"
#include <unordered_map>
#include <fstream>

#include <iostream>

std::string file_utils::user_avatar_storage_path = "/data/avatar/user/";
std::string file_utils::server_avatar_storage_path = "/data/avatar/server/";

std::string file_utils::get_image_ext(const std::string_view& fraw)
{
	static const std::vector<std::pair<std::string_view, std::string> > sig_list = {
		{"\x42\x4D", "bmp"},

		{"\xFF\xD8\xFF\xE0", "jpg"},
		{"\xFF\xD8\xFF\xEE", "jpg"},
		{"\xFF\xD8\xFF\xE1", "jpg"},

		{"\x89PNG\x0D\x0A\x1A\x0A", "png"}
	};
	for(auto it = sig_list.begin(); it != sig_list.end(); ++it){
		if(fraw.size() >= it->first.size() && fraw.substr(0, it->first.size()) == it->first)
			return it->second;
	}
	return "";
}

void file_utils::save_file(const std::string_view& fraw, std::string fpath)
{ // TODO maybe launch a task instead, moving fraw to somewhere
	std::ofstream fd(fpath);
	fd.write(fraw.data(), fraw.size());
}


std::shared_ptr<http_response> file_utils::parse_user_avatar(const http_request& req, std::string arg_name,
								int id, std::string& out_fname)
{
	std::string_view fraw = req.get_arg_flat(arg_name);
	std::string ext = get_image_ext(fraw);
	if(!ext.size())
		return create_response::string("Image has invalid format", 400);
	out_fname = std::to_string(id) + "." + ext;
	save_file(fraw, user_avatar_storage_path + out_fname);
	
	return nullptr;
}
std::shared_ptr<http_response> file_utils::parse_server_avatar(const http_request& req, std::string arg_name,
								int id, std::string& out_fname)
{
	std::string_view fraw = req.get_arg_flat(arg_name);
	std::string ext = get_image_ext(fraw);
	if(!ext.size())
		return create_response::string("Image has invalid format", 400);
	out_fname = std::to_string(id) + "." + ext;
	save_file(fraw, server_avatar_storage_path + out_fname);
	
	return nullptr;
}
