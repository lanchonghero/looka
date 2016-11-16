#ifndef _LOOKA_CONFIG_INDEX_HPP
#define _LOOKA_CONFIG_INDEX_HPP
#include "looka_config_parser.hpp"

class LookaConfigIndex
{
public:
  LookaConfigIndex(LookaConfigParser* lc, const std::string& secName="");
  virtual ~LookaConfigIndex() {}

public:
  std::string source;
  std::string dict_path;
  std::string index_path;

  std::string summary_file_uint;
  std::string summary_file_float;
  std::string summary_file_multi;
  std::string summary_file_string;
  std::string index_file;

  static std::string mSectionTag;
  std::string  mSectionName;
};

#endif //_LOOKA_CONFIG_INDEX_HPP
