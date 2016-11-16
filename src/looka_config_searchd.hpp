#ifndef _LOOKA_CONFIG_SEARCHD_HPP
#define _LOOKA_CONFIG_SEARCHD_HPP
#include "looka_config_parser.hpp"

class LookaConfigSearchd
{
public:
  LookaConfigSearchd(LookaConfigParser* lc, const std::string& secName="");
  virtual ~LookaConfigSearchd() {}

public:
  int listen_port;
  int thread_num;
  int max_matches;
  int client_timeout;
  int read_timeout;
  std::string searchd_log;
  std::string query_log;
  std::string pid_file;
  
  static std::string  mSectionTag;

private:
  std::string  mSectionName;
};


#endif //_LOOKA_CONFIG_SEARCHD_HPP
