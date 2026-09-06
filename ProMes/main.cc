#include <drogon/drogon.h>
#include <drogon/orm/DbClient.h>
#include <iostream>
#include <string>
#include <memory>
#include <future>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <chrono>
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>
#include <locale>
#include <ctime>
#include <iterator>
#include <utility>


 std::string require_env (const char* env_char){
    if (const char* env_p = std::getenv(env_char)){
        std::cout << "Your env variables are" << env_p << '\n';
        return env_p;
        }
      std::cerr <<"Missing env var :" << env_char << "\n";
      std::exit(1);
  }

  std::string today_date(){

  std::time_t time = std::time({});
  char timeString[std::size("yyyy-mm-dd")];

  std::strftime(std::data(timeString),std::size(timeString),"%F",std::localtime(&time));

  return timeString;

   }


int clock_reader(){
   std::time_t rawtime;
   struct tm * timeinfo;

   char buffer [80];
   time (&rawtime);

   timeinfo = localtime(&rawtime);

   return timeinfo->tm_hour * 100 + timeinfo -> tm_min;
  }


int main(){

  std::string sid = require_env("TSID");
  std::string auth_token = require_env("TTOKEN");
  std::string from_number = require_env("TNUMBER");
  std::string db_connect = require_env("DB_URL");
  std::string content_sid = require_env("CONTENT_SID");

  std::cout << today_date() << std::endl;

  auto dbClient = drogon::orm::DbClient::newPgClient(db_connect,4);

   std::string to_number = "whatsapp:+61405245648"; //to what number

  auto client =drogon::HttpClient::newHttpClient("https://api.twilio.com"); //a HTTP client to twillio

  std::string t_path = "/2010-04-01/Accounts/" + sid + "/Messages.json"; //set the path
  std::string cred = sid + ":" + auth_token;
  std::string cred_encoded = drogon::utils::base64Encode(cred);
  std::string login = "Basic " + cred_encoded;

  std::vector<std::pair<std::string, std::string>> morning_tasks;
  std::vector<std::pair<std::string, std::string>> noon_tasks;
  std::vector<std::pair<std::string, std::string>> evening_tasks;
  std::vector<std::pair<std::string, std::string>> night_tasks;
  std::vector<std::pair<std::string, std::string>> tasks;


  //Morning List
  morning_tasks.push_back({"Have you taken B6, Creatine & D3?","morning_tasks"});
  noon_tasks.push_back({"Have you gone to the gym today?","noon_tasks"});
  night_tasks.push_back({"Have you recorded footage on instagram today?","night_tasks"});
  evening_tasks.push_back({"Have you learned a new concept in programming today?","evening_tasks"});



//Message every 200 seconds
 drogon::app().getLoop()->runEvery(std::chrono::seconds(60),[client,t_path,to_number,from_number,tasks,login,dbClient,content_sid,morning_tasks,noon_tasks,night_tasks,evening_tasks](){

     int now = clock_reader();

     std::vector<std::pair<std::string, std::string>> slots_tasks;

     if (now == 245) slots_tasks = morning_tasks;
     else if (now == 1517) slots_tasks = noon_tasks;
     else if (now == 1518) slots_tasks = evening_tasks;
     else if (now == 1519) slots_tasks = night_tasks;
     else return;

     dbClient-> execSqlAsync("SELECT * FROM entries WHERE entry_date = $1 AND question_key = $2;",[client,t_path,to_number,from_number,tasks,login,dbClient,content_sid,slots_tasks](const drogon::orm::Result &result){
         if (!result.empty()){
         std::cout << "Already ran today" <<std::endl;
         return;
         }
     auto req = drogon::HttpRequest::newHttpFormPostRequest();
     req->setPath(t_path);
     req->setParameter("To", to_number);
     req->setParameter("ContentSid",content_sid);
     std::string start = "{\"1\":\"" + slots_tasks[0].first + "\"}";
     req->setParameter("ContentVariables",start);
     req->setParameter("From", from_number);
     req->addHeader("Authorization",login);

      client->sendRequest(req,[](drogon::ReqResult result, const drogon::HttpResponsePtr &response){
         if (result != drogon::ReqResult::Ok){
          std::cout
          << "error while sending request to server! result: "
          << result << std::endl;
          return;
        }
        std::cout << "receive response!" << std::endl;
        std::cout << response->getBody() << std::endl;
     });

      for (int i = 0; i < slots_tasks.size(); i++){
          dbClient->execSqlAsync("INSERT INTO entries (entry_date,question_key) VALUES ($1,$2);",[](const drogon::orm::Result &result){
          std::cout << "Insert OK" << std::endl;
      },[](const drogon::orm::DrogonDbException &e){
          std::cerr << "error: " << e.base().what() << std::endl;
      },today_date(),slots_tasks[i].second);
      }

     },[](const drogon::orm::DrogonDbException &e){
     std::cerr << "error " << e.base().what() << std::endl;
     },today_date(),slots_tasks[0].second);
 });

 //Receiving tasks
drogon::app().registerHandler(
    "/whatsapp",[dbClient,tasks,client,t_path,to_number,from_number,login,content_sid](const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback){
    drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse();
    std::string body = req->getParameter("Body");

    std::transform(body.begin(), body.end(), body.begin(),::tolower);

    if (body == "yes"){
         dbClient->execSqlAsync("SELECT * FROM entries where entry_date = $1 AND answer_bool IS NULL ORDER BY id LIMIT 1;",[dbClient,client,t_path,to_number,from_number,login,tasks,content_sid](const drogon::orm::Result &result){
             if (result.empty()){
             std::cout << "Nothing is pending" << std::endl;
                  return;
             }
         std::string pending_key = result[0]["question_key"].as<std::string>();
         int pending_id = result[0]["id"].as<int>();
         dbClient->execSqlAsync("UPDATE entries SET answer_bool = $1 WHERE id = $2;",[dbClient,pending_key,client,t_path,to_number,from_number,login,tasks,content_sid](const drogon::orm::Result &r){
             std::cout << "FILLED" << pending_key << std::endl;
             dbClient->execSqlAsync("SELECT * FROM entries where entry_date = $1 AND answer_bool IS NULL ORDER BY id LIMIT 1;",[client,t_path,to_number,from_number,login,tasks,content_sid](const drogon::orm::Result &result){
                 if (result.empty()){
                 auto req = drogon::HttpRequest::newHttpFormPostRequest();
                     req->setPath(t_path);
                     req->setParameter("To", to_number);
                     req->setParameter("From", from_number);
                     req->addHeader("Authorization",login);
                    req->setParameter("Body","All Tasks are finished, well done!");
                   client->sendRequest(req,[](drogon::ReqResult result, const drogon::HttpResponsePtr &response){
                      if (result != drogon::ReqResult::Ok){
                          std::cout << "error while sending request to server! result: " << result << std::endl;
                         return;
                       }
                        std::cout << "receive response!" << std::endl;
                        std::cout << response->getBody() << std::endl;
                       });
                 return;
                 }else{
                   std::string next_key = result[0]["question_key"].as<std::string>();
                   std::string next_text;
                   for (int i = 0; i < tasks.size(); i++){
                      if (tasks[i].second == next_key){
                       next_text = tasks[i].first;
                       break;
                      }
                     }
                   if (next_text.empty()){
                   std::cerr << "Missing key" << std::endl;
                    return;
                   }
                     auto req = drogon::HttpRequest::newHttpFormPostRequest();
                     req->setPath(t_path);
                     req->setParameter("To", to_number);
                     req->setParameter("From", from_number);
                     req->addHeader("Authorization",login);
                     req->setParameter("ContentSid",content_sid);
                     std::string start = "{\"1\":\"" + next_text + "\"}";
                     req->setParameter("ContentVariables",start);
                     client->sendRequest(req,[](drogon::ReqResult result, const drogon::HttpResponsePtr &response){
                      if (result != drogon::ReqResult::Ok){
                          std::cout << "error while sending request to server! result: " << result << std::endl;
                         return;
                       }
                        std::cout << "receive response!" << std::endl;
                        std::cout << response->getBody() << std::endl;
                       });
                }
                 },[](const drogon::orm::DrogonDbException &e){
                   std::cerr <<"error " << e.base().what() << std::endl;
                 },today_date());
             },
             [](const drogon::orm::DrogonDbException &e){
                std::cerr << "error " <<e.base().what() << std::endl;
             },true,pending_id);
         },[](const drogon::orm::DrogonDbException &e){
         std::cerr << "error " << e.base().what() << std::endl;
         },today_date());
    }
    else if (body == "no"){
      dbClient->execSqlAsync("INSERT INTO entries (entry_date,question_key,answer_bool) VALUES ($1,$2,$3);",[](const drogon::orm::Result &result){
         std::cout << "Insert OK" << std::endl;
      },
      [](const drogon::orm::DrogonDbException &e){
        std::cerr << "error: " << e.base().what() << std::endl;
      },today_date(),tasks[0].second,false);
    } else{
      std::cerr << "Unrecognised reply" << body << std:: endl;
    }
    std::cout << "Message:" << body << std::endl;
    callback(resp);
    });
    drogon::app().addListener("0.0.0.0", 5555);
    drogon::app().run();
    return 0;
}
