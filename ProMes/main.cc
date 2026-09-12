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
#include <helpers/helper.h>
#include "pmconfig/config.h"

using namespace std;
int main(){

 Config cfg = load_config();
 string currDate = Helper::today_date();
int clock = Helper::clock_reader();

  auto dbClient = drogon::orm::DbClient::newPgClient(cfg.db_connect,4);

  auto client = drogon::HttpClient::newHttpClient("https://api.twilio.com"); //a HTTP client to twillio

  string t_path = "/2010-04-01/Accounts/" + cfg.sid + "/Messages.json"; //set the path
  string cred = cfg.sid + ":" + cfg.auth_token;
  string cred_encoded = drogon::utils::base64Encode(cred);
  string login = "Basic " + cred_encoded;

  vector<pair<string, string>> morning_tasks;
  vector<pair<string, string>> noon_tasks;
  vector<pair<string, string>> evening_tasks;
  vector<pair<string, string>> night_tasks;
  vector<pair<string, string>> tasks;


  //Morning List
  morning_tasks.push_back({"Have you taken B6, Creatine & D3?","morning_tasks"});
  noon_tasks.push_back({"Have you gone to the gym today?","noon_tasks"});
  night_tasks.push_back({"Have you recorded footage on instagram today?","night_tasks"});
  evening_tasks.push_back({"Have you learned a new concept in programming today?","evening_tasks"});



//Message every 200 seconds
 drogon::app().getLoop()->runEvery(chrono::seconds(5),[cfg,client,t_path,tasks,login,dbClient,morning_tasks,noon_tasks,night_tasks,evening_tasks,currDate,clock](){

     vector<pair<string, string>> slots_tasks;

     if (clock == 1730) slots_tasks = morning_tasks;
     else if (clock == 1517) slots_tasks = noon_tasks;
     else if (clock == 1518) slots_tasks = evening_tasks;
     else if (clock == 1519) slots_tasks = night_tasks;
     else return;

     dbClient-> execSqlAsync("SELECT * FROM entries WHERE entry_date = $1 AND question_key = $2;",[cfg,client,t_path,tasks,login,dbClient,slots_tasks,currDate](const drogon::orm::Result &result){
         if (!result.empty()){
         cout << "Already ran today" <<endl;
         return;
         }
     auto req = drogon::HttpRequest::newHttpFormPostRequest();
     req->setPath(t_path);
     req->setParameter("To", cfg.to_number);
     req->setParameter("ContentSid",cfg.content_sid);
     string start = "{\"1\":\"" + slots_tasks[0].first + "\"}";
     req->setParameter("ContentVariables",start);
     req->setParameter("From", cfg.from_number);
     req->addHeader("Authorization",login);

      client->sendRequest(req,[](drogon::ReqResult result, const drogon::HttpResponsePtr &response){
         if (result != drogon::ReqResult::Ok){
          cout
          << "error while sending request to server! result: "
          << result << endl;
          return;
        }
        cout << "receive response!" << endl;
        cout << response->getBody() << endl;
     });

      for (int i = 0; i < slots_tasks.size(); i++){
          dbClient->execSqlAsync("INSERT INTO entries (entry_date,question_key) VALUES ($1,$2);",[](const drogon::orm::Result &result){
          cout << "Insert OK" << endl;
      },[](const drogon::orm::DrogonDbException &e){
          cerr << "error: " << e.base().what() << endl;
      },currDate,slots_tasks[i].second);
      }

     },[](const drogon::orm::DrogonDbException &e){
     cerr << "error " << e.base().what() << endl;
     },currDate,slots_tasks[0].second);
 });

 //Receiving tasks
drogon::app().registerHandler(
    "/whatsapp",[cfg,dbClient,tasks,client,t_path,login,currDate](const drogon::HttpRequestPtr &req, function<void(const drogon::HttpResponsePtr &)> &&callback){
    drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse();
    string body = req->getParameter("Body");

    transform(body.begin(), body.end(), body.begin(),::tolower);

    if (body == "yes"){
         dbClient->execSqlAsync("SELECT * FROM entries where entry_date = $1 AND answer_bool IS NULL ORDER BY id LIMIT 1;",[cfg,dbClient,client,t_path,login,tasks,currDate](const drogon::orm::Result &result){
             if (result.empty()){
             cout << "Nothing is pending" << endl;
                  return;
             }
         string pending_key = result[0]["question_key"].as<string>();
         int pending_id = result[0]["id"].as<int>();
         dbClient->execSqlAsync("UPDATE entries SET answer_bool = $1 WHERE id = $2;",[cfg,dbClient,pending_key,client,t_path,login,tasks,currDate](const drogon::orm::Result &r){
             cout << "FILLED" << pending_key << endl;
             dbClient->execSqlAsync("SELECT * FROM entries where entry_date = $1 AND answer_bool IS NULL ORDER BY id LIMIT 1;",[cfg,client,t_path,login,tasks,currDate](const drogon::orm::Result &result){
                 if (result.empty()){
                 auto req = drogon::HttpRequest::newHttpFormPostRequest();
                     req->setPath(t_path);
                     req->setParameter("To", cfg.to_number);
                     req->setParameter("From", cfg.from_number);
                     req->addHeader("Authorization",login);
                    req->setParameter("Body","All Tasks are finished, well done!");
                   client->sendRequest(req,[](drogon::ReqResult result, const drogon::HttpResponsePtr &response){
                      if (result != drogon::ReqResult::Ok){
                          cout << "error while sending request to server! result: " << result << endl;
                         return;
                       }
                        cout << "receive response!" << endl;
                        cout << response->getBody() << endl;
                       });
                 return;
                 }else{
                   string next_key = result[0]["question_key"].as<string>();
                   string next_text;
                   for (int i = 0; i < tasks.size(); i++){
                      if (tasks[i].second == next_key){
                       next_text = tasks[i].first;
                       break;
                      }
                     }
                   if (next_text.empty()){
                   cerr << "Missing key" << endl;
                    return;
                   }
                     auto req = drogon::HttpRequest::newHttpFormPostRequest();
                     req->setPath(t_path);
                     req->setParameter("To", cfg.to_number);
                     req->setParameter("From", cfg.from_number);
                     req->addHeader("Authorization",login);
                     req->setParameter("ContentSid",cfg.content_sid);
                     string start = "{\"1\":\"" + next_text + "\"}";
                     req->setParameter("ContentVariables",start);
                     client->sendRequest(req,[](drogon::ReqResult result, const drogon::HttpResponsePtr &response){
                      if (result != drogon::ReqResult::Ok){
                          cout << "error while sending request to server! result: " << result << endl;
                         return;
                       }
                        cout << "receive response!" << endl;
                        cout << response->getBody() << endl;
                       });
                }
                 },[](const drogon::orm::DrogonDbException &e){
                   cerr <<"error " << e.base().what() << endl;
                 },currDate);
             },
             [](const drogon::orm::DrogonDbException &e){
                cerr << "error " <<e.base().what() << endl;
             },true,pending_id);
         },[](const drogon::orm::DrogonDbException &e){
         cerr << "error " << e.base().what() << endl;
         },currDate);
    }
    else if (body == "no"){
      dbClient->execSqlAsync("INSERT INTO entries (entry_date,question_key,answer_bool) VALUES ($1,$2,$3);",[](const drogon::orm::Result &result){
         cout << "Insert OK" << endl;
      },
      [](const drogon::orm::DrogonDbException &e){
        cerr << "error: " << e.base().what() << endl;
      },currDate,tasks[0].second,false);
    } else{
      cerr << "Unrecognised reply" << body <<  endl;
    }
    cout << "Message:" << body << endl;
    callback(resp);
    });
    drogon::app().addListener("0.0.0.0", 5555);
    drogon::app().run();
    return 0;
}
