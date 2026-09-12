#include <string>
#include <iostream>

struct Config {
  std::string from_number;
  std::string sid;
  std::string auth_token;
  std::string db_connect;
  std::string content_sid;
  std::string to_number;
};

Config load_config();


