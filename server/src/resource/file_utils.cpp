#include "file_utils.h"
#include <unordered_map>
#include <fstream>
#include <random>
#include <cstring>

#include <iostream>

std::string file_utils::user_avatar_storage_path = "/data/avatar/user/";
std::string file_utils::server_avatar_storage_path = "/data/avatar/server/";
std::string file_utils::file_storage_path = "/data/upload/";

size_t file_utils::file_storage_size = 1024 * 100;

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
std::string file_utils::generate_fname(size_t sz)
{
	static const char chars[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIKLMNOPQRSTUVWXYZ";
	static std::random_device dev;
	static std::mt19937 rng(dev());
	static std::uniform_int_distribution<std::mt19937::result_type> rdist(0, strlen(chars) - 1);

	std::string s; s.reserve(sz);
	for(size_t i = 0; i < sz; ++i)
		s += chars[rdist(rng)];
	return s;
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


void file_utils::fs_add_busy(pqxx::work& tx, int user_id, size_t bytes)
{
	tx.exec("UPDATE users SET fs_busy = fs_busy + $1 WHERE user_id = $2", pqxx::params(bytes, user_id));
	tx.commit();
}
void file_utils::fs_sub_busy(pqxx::work& tx, int user_id, size_t bytes)
{
	tx.exec("UPDATE users SET fs_busy = fs_busy - $1 WHERE user_id = $2", pqxx::params(bytes, user_id));
	tx.commit();
}

size_t file_utils::delete_oldest_file(pqxx::work& tx, int user_id)
{
	std::filesystem::directory_entry oldest_fd;
	for(auto const& fd : std::filesystem::directory_iterator(file_storage_path / std::filesystem::path(std::to_string(user_id)))){
		if(!oldest_fd.exists() || fd.last_write_time() < oldest_fd.last_write_time())
			oldest_fd = fd;
	}
	size_t sz = oldest_fd.file_size();
	std::filesystem::remove(oldest_fd);
	fs_sub_busy(tx, user_id, sz);
	return sz;
}
std::shared_ptr<http_response> file_utils::fs_make_space(pqxx::work& tx, int user_id, size_t bytes)
{
	pqxx::result r = tx.exec("SELECT COUNT(*) FROM users");
	int user_count = r[0]["count"].as<int>();
	size_t storage_per_user = file_storage_size / user_count;
	if(bytes >= storage_per_user)
		return create_response::string("File size is bigger than user storage size", 400);

	// Free total storage if it overflows
	r = tx.exec("SELECT total FROM fs_total_busy");
	long long total_busy = r[0]["total"].as<int>();
	long long tofree_total = total_busy > file_storage_size ? total_busy - file_storage_size : 0;


	if(tofree_total > 0){
		r = tx.exec("SELECT * FROM users WHERE fs_busy > $1", pqxx::params(storage_per_user));
		for(size_t i = 0; i < r.size(); ++i){ // try to free space by rolling over other users' storages that are overflowing
			const pqxx::row& _row = r[i];
			size_t overflow = _row["fs_busy"].as<int>() - storage_per_user;
			int _id = _row["user_id"].as<int>();
			for(;;){
				size_t deleted = delete_oldest_file(tx, _id);
				if(!deleted)
					break;
				if(deleted > overflow){
					tofree_total -= overflow;
					break;
				} else {
					overflow -= deleted;
					tofree_total -= deleted;
				}
			}
		}
	
		if(tofree_total > 0){
			std::cerr << "PANIC: cannot free " << tofree_total << " total fs bytes" << std::endl;
			return create_response::string("Internal Server Error", 500);
		}
	}

	// Free user storage if it overflows
	r = tx.exec("SELECT fs_busy FROM users WHERE user_id = $1", pqxx::params(user_id));
	long long user_busy = r[0]["fs_busy"].as<int>();
	long long tofree_user = user_busy > storage_per_user ? (user_busy - storage_per_user + bytes) :
				(storage_per_user - user_busy > bytes ? 0 : bytes - (storage_per_user - user_busy));
	if(tofree_user > 0){
		while(tofree_user > 0){
			size_t deleted = delete_oldest_file(tx, user_id);
			if(!deleted)
				break;
			tofree_user -= deleted;
		}

		if(tofree_user > 0){
			std::cerr << "PANIC: cannot free " << tofree_user << " user fs bytes" << std::endl;
			return create_response::string("Internal Server Error", 500);
		}	
	}

	return nullptr;
}
