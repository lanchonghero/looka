#include <fstream>
#include "looka_config_source.hpp"
#include "looka_string_utils.hpp"
#include "looka_log.hpp"

std::string LookaConfigSource::mSectionTag = "source";

LookaConfigSource::LookaConfigSource(LookaConfigParser* lc, const std::string& secName)
{
  mSectionName = secName;

  std::string item;
  std::string conf_str;
  item = "sql_host";
  if ((sql_host = lc->GetString(mSectionTag, mSectionName, item, "")) == "")
    _ERROR_EXIT(-1, "[LookaConfigSource Init Error] [get %s failed]", item.c_str());

  item = "sql_user";
  if ((sql_user = lc->GetString(mSectionTag, mSectionName, item, "")) == "")
    _ERROR_EXIT(-1, "[LookaConfigSource Init Error] [get %s failed]", item.c_str());

  item = "sql_pass";
  sql_pass = lc->GetString(mSectionTag, mSectionName, item, "");

  item = "sql_db";
  if ((sql_db = lc->GetString(mSectionTag, mSectionName, item, "")) == "")
    _ERROR_EXIT(-1, "[LookaConfigSource Init Error] [get %s failed]", item.c_str());

  item = "sql_query";
  if ((sql_query = lc->GetString(mSectionTag, mSectionName, item, "")) == "")
    _ERROR_EXIT(-1, "[LookaConfigSource Init Error] [get %s failed]", item.c_str());
  
  item = "sql_socket";
  sql_socket = lc->GetString(mSectionTag, mSectionName, item, "");

  item = "sql_print_query";
  conf_str = lc->GetString(mSectionTag, mSectionName, item, "false");
  sql_print_query = (conf_str == "true");

  item = "sql_port";
  if ((sql_port = lc->GetInt(mSectionTag, mSectionName, item, 0)) == 0)
    _ERROR_EXIT(-1, "[LookaConfigSource Init Error] [get %s failed]", item.c_str());

  sql_attr_uint = lc->GetStringV(mSectionTag, mSectionName, "sql_attr_uint");
  sql_attr_float = lc->GetStringV(mSectionTag, mSectionName, "sql_attr_float");
  sql_attr_multi = lc->GetStringV(mSectionTag, mSectionName, "sql_attr_multi");
  sql_attr_string = lc->GetStringV(mSectionTag, mSectionName, "sql_attr_string");
  sql_field_string = lc->GetStringV(mSectionTag, mSectionName, "sql_field_string");

  item = "sql_query_pre";
  sql_query_pre = lc->GetString(mSectionTag, mSectionName, item, "");
  splitString(sql_query_pre, ';', sql_query_pre_set);
  
  item = "sql_query_post";
  sql_query_post = lc->GetString(mSectionTag, mSectionName, item, "");
  splitString(sql_query_post, ';', sql_query_post_set);

  item = "sql_query_pre_path";
  sql_query_pre_path = lc->GetString(mSectionTag, mSectionName, item, "");
  if (!sql_query_pre_path.empty())
    AppendSqlFromFile(sql_query_pre_set, sql_query_pre_path);

  item = "sql_query_post_path";
  sql_query_post_path = lc->GetString(mSectionTag, mSectionName, item, "");
  if (!sql_query_post_path.empty())
    AppendSqlFromFile(sql_query_post_set, sql_query_post_path);
}

void LookaConfigSource::AppendSqlFromFile(
  std::vector<std::string>& sqls,
  const std::string& filename)
{
  std::ifstream f(filename.c_str());
  if (!f) {
    _ERROR("[cannot open file %s]", filename.c_str());
    return;
  }

  std::string s, line;
  while (getline(f, line)) {
    if (line.empty())
      continue;
    if (line.find(";") != std::string::npos) {
      s += line;
      s = rtrimc(rtrim(s), ';');
      sqls.push_back(s);
      s = "";
    } else {
      s += line;
    }
  }
  f.close();
}
