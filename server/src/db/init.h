#ifndef DB_INIT_H
#define DB_INIT_H

#include <string>

void db_connect(std::string conn_str); // retries endlessly until db connection can be established
void db_init(std::string conn_str);

void db_create_test(std::string conn_str);

#endif
