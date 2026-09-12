#include "helper.h"
#include <cstdlib>
#include <iostream>
#include <string>
#include <locale>

using namespace std;

string Helper::require_env(const char* env_char){

    if (const char* env_p = std::getenv(env_char)){
        std::cout << "Your env variables are" << env_p << '\n';
        return env_p;
        }
      std::cerr <<"Missing env var :" << env_char << "\n";
      std::exit(1);
};

string Helper::today_date(){

  std::time_t time = std::time({});
  char timeString[std::size("yyyy-mm-dd")];

  std::strftime(std::data(timeString),std::size(timeString),"%F",std::localtime(&time));

  return timeString;
};

int Helper::clock_reader(){

   std::time_t rawtime;
   struct tm * timeinfo;

   char buffer [80];
   time (&rawtime);

   timeinfo = localtime(&rawtime);

   return timeinfo->tm_hour * 100 + timeinfo -> tm_min;
};

