#include "looka_config_searchd.hpp"
#include "looka_log.hpp"

std::string LookaConfigSearchd::mSectionTag = "searchd";

LookaConfigSearchd::LookaConfigSearchd(LookaConfigParser* lc, const std::string& secName)
{
  mSectionName = secName;

  std::string item;
  item = "listen";
  if ((listen_port = lc->GetInt(mSectionTag, mSectionName, item, 0)) == 0)
    _ERROR_EXIT(-1, "[LookaConfigSearchd Init Error] [get %s failed]", item.c_str());
  
  item = "thread_num";
  if ((thread_num = lc->GetInt(mSectionTag, mSectionName, item, 0)) == 0)
    _ERROR_EXIT(-1, "[LookaConfigSearchd Init Error] [get %s failed]", item.c_str());

  item = "searchd_log";
  if ((searchd_log = lc->GetString(mSectionTag, mSectionName, item, "")) == "")
    _ERROR_EXIT(-1, "[LookaConfigSearchd Init Error] [get %s failed]", item.c_str());

  item = "query_log";
  if ((query_log = lc->GetString(mSectionTag, mSectionName, item, "")) == "")
    _ERROR_EXIT(-1, "[LookaConfigSearchd Init Error] [get %s failed]", item.c_str());

  item = "pid_file";
  if ((pid_file = lc->GetString(mSectionTag, mSectionName, item, "")) == "")
    _ERROR_EXIT(-1, "[LookaConfigSearchd Init Error] [get %s failed]", item.c_str());

  item = "role";
  if ((role = lc->GetString(mSectionTag, mSectionName, item, "")) == "")
    _ERROR_EXIT(-1, "[LookaConfigSearchd Init Error] [get %s failed]", item.c_str());

  item = "max_matches";
  if ((max_matches = lc->GetInt(mSectionTag, mSectionName, item, 0)) == 0)
    _ERROR_EXIT(-1, "[LookaConfigSearchd Init Error] [get %s failed]", item.c_str());

  item = "client_timeout";
  if ((client_timeout = lc->GetInt(mSectionTag, mSectionName, item, 0)) == 0)
    _ERROR_EXIT(-1, "[LookaConfigSearchd Init Error] [get %s failed]", item.c_str());

  item = "read_timeout";
  if ((read_timeout = lc->GetInt(mSectionTag, mSectionName, item, 0)) == 0)
    _ERROR_EXIT(-1, "[LookaConfigSearchd Init Error] [get %s failed]", item.c_str());
}
