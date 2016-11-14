#include "looka_config_index.hpp"
#include "looka_log.hpp"
  
std::string LookaConfigIndex::mSectionTag = "index";

LookaConfigIndex::LookaConfigIndex(LookaConfigParser* lc, const std::string& secName)
{
  mSectionName = secName;

  std::string item;
  item = "source";
  if ((source = lc->GetString(mSectionTag, mSectionName, item, "")) == "")
    _ERROR_EXIT(-1, "[LookaConfigIndex Init Error] [get %s failed]", item.c_str());
  
  item = "dict_path";
  if ((dict_path = lc->GetString(mSectionTag, mSectionName, item, "")) == "")
    _ERROR_EXIT(-1, "[LookaConfigIndex Init Error] [get %s failed]", item.c_str());
  
  item = "index_path";
  if ((index_path = lc->GetString(mSectionTag, mSectionName, item, "")) == "")
    _ERROR_EXIT(-1, "[LookaConfigIndex Init Error] [get %s failed]", item.c_str());

  if (index_path.find_last_of('/') != index_path.length() - 1)
    index_path += "/";
  std::string name = (secName.empty()) ? "looka" : secName;

  summary_file_uint = index_path + name + ".lcu";
  summary_file_float = index_path + name + ".lcf";
  summary_file_multi = index_path + name + ".lcm";
  summary_file_string = index_path + name + ".lcs";
  index_file = index_path + name + ".lci";
}
