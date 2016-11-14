#ifndef _LOOKA_CONFIG_SOURCE_HPP
#define _LOOKA_CONFIG_SOURCE_HPP
#include "looka_config_parser.hpp"

class LookaConfigSource
{
public:
  LookaConfigSource(LookaConfigParser* lc, const std::string& secName="");
  virtual ~LookaConfigSource() {}

private:
  void AppendSqlFromFile(
    std::vector<std::string>& sqls,
    const std::string& filename);

public:
  std::string sql_host;
  std::string sql_user;
  std::string sql_pass;
  std::string sql_db;
  std::string sql_table;
  std::string sql_socket;
  std::string sql_query;
  std::string sql_query_pre;
  std::string sql_query_post;
  std::string sql_query_pre_path;
  std::string sql_query_post_path;
  int sql_port;
  bool sql_print_query;

  std::vector<std::string> sql_attr_uint;
  std::vector<std::string> sql_attr_float;
  std::vector<std::string> sql_attr_string;
  std::vector<std::string> sql_attr_multi;
  std::vector<std::string> sql_field_string;

  std::vector<std::string> sql_query_pre_set;
  std::vector<std::string> sql_query_post_set;

  static std::string  mSectionTag;

private:
  std::string  mSectionName;
};

#endif //_LOOKA_CONFIG_SOURCE_HPP
