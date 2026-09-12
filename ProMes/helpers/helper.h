#include <string>

class Helper{

public:
  static std::string require_env(const char* env_char);
  static std::string today_date();
  static int clock_reader();
};
