#include <drogon/drogon.h>
#include <iostream>
#include <string>
#include <memory>
#include <future>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <chrono>
#include <vector>


 std::string require_env (const char* env_char){
    if (const char* env_p = std::getenv(env_char)){
        std::cout << "Your env variables are" << env_p << '\n';
        return env_p;
        }
      std::cerr <<"Missing env var :" << env_char << "\n";
      std::exit(1);
  }

int main(){

  std::string sid = require_env("TSID");
  std::string auth_token = require_env("TTOKEN");
  std::string from_number = require_env("TNUMBER");
  std::string db_connect = require_env("DB_URL");

  std::string to_number = "whatsapp:+61405245648"; //to what number

  auto client =drogon::HttpClient::newHttpClient("https://api.twilio.com"); //a HTTP client to twillio

  std::string t_path = "/2010-04-01/Accounts/" + sid + "/Messages.json"; //set the path
  std::string cred = sid + ":" + auth_token;
  std::string cred_encoded = drogon::utils::base64Encode(cred);
  std::string login = "Basic " + cred_encoded;

  std::vector<std::string> tasks;

  tasks.push_back("Have you taken B6, Creatine & D3? [Yes/No]");
  tasks.push_back("Have you brushed your teeth thoroughly with e.toothbrush for 3 mins? [Yes/No]");
  tasks.push_back("Have you drank 2 glasses of water? [Yes/No]");
  tasks.push_back("Have you meditated for 15 minutes in the train? [Yes/No]");

  std::string app_task;

  for (const std::string &task:tasks){
    app_task+= task;
    app_task+= "\n";
  }

  std::cout << app_task << std::endl;

  auto req = drogon::HttpRequest::newHttpFormPostRequest();
  req->setPath(t_path);
  req->setParameter("To",to_number);
  req->setParameter("From",from_number);
  req->setParameter("Body", app_task);
  req->addHeader("Authorization",login);

//Message every 200 seconds
 drogon::app().getLoop()->runEvery(std::chrono::seconds(10),[client,t_path,to_number,from_number,app_task,login,req](){
     client->sendRequest(req,[](drogon::ReqResult result, const drogon::HttpResponsePtr &response){
         if (result != drogon::ReqResult::Ok){
          std::cout
          << "error while sending request to server! result: "
          << result << std::endl;
          return;
        }
        std::cout << "receive response!" << std::endl;
        std::cout << response->getBody() << std::endl;
        auto cookies = response -> cookies();
     });
     std::cout << "tick" << std::endl;
 });

 //Receiving tasks
drogon::app().registerHandler(
    "/whatsapp",[](const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback){
    drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse();
    std::string body = req->getParameter("Body");
    resp->setBody("");
    std::cout << "Who knocked?" << std::endl;
    std::cout << "Message:" << body << std::endl;
    callback(resp);
    });

    //build the connection first
    //Set HTTP listener address and port
    drogon::app().addListener("0.0.0.0", 5555);

    //Load config file
    //drogon::app().loadConfigFile("../config.json");
    //drogon::app().loadConfigFile("../config.yaml");
    //Run HTTP framework,the method will block in the internal event loops
    drogon::app().run();
    return 0;
}
