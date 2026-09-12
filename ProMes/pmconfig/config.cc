#include "config.h"
#include "helpers/helper.h"


Config load_config(){
  Config cfg;
  cfg.from_number = Helper::require_env("TNUMBER");
  cfg.sid = Helper::require_env("TSID");
  cfg.auth_token = Helper::require_env("TTOKEN");
  cfg.db_connect = Helper::require_env("DB_URL");
  cfg.content_sid = Helper::require_env("CONTENT_SID");
  cfg.to_number = "whatsapp:+61405245648";
  return cfg;
}


