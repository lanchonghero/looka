#include <fstream>
#include <sstream>
#include <algorithm>
#include "looka_config_parser.hpp"
#include "looka_log.hpp"

LookaConfigParser::LookaConfigParser()
{
}

LookaConfigParser::~LookaConfigParser()
{
}

int LookaConfigParser::LoadConfig(const std::string& filename)
{
  std::ifstream f(filename.c_str());
  if (!f)
    _ERROR_RETURN(-1, "[cannot open configuration file %s]", filename.c_str());
  
  std::stringstream ss;
  std::string line;
  while (getline(f, line)) {
    DelComment(line);
    Trim(line);
    if (!line.empty()) ss << line << std::endl;
  }

  if (ParsePureConfig(ss.str())) {
    f.close();
    _ERROR_RETURN(-1, "[parse config failed]");
  }

  f.close();
  return 0;
}

std::vector<std::string> LookaConfigParser::GetSectionTags()
{
  std::vector<std::string> sectionTags;
  std::map<std::string, std::vector<SectionInfo> >::iterator it;
  for (it=m_sec_info.begin(); it!=m_sec_info.end(); ++it)
    sectionTags.push_back(it->first);
  return sectionTags;
}

std::vector<std::string> LookaConfigParser::GetSectionNames(const std::string& secTag)
{
  std::vector<std::string> sectionNames;
  std::map<std::string, std::vector<SectionInfo> >::iterator it;
  for (it=m_sec_info.begin(); it!=m_sec_info.end(); ++it) {
    if (it->first == secTag) {
      std::vector<SectionInfo>& vInfo = it->second;
      for (unsigned int i=0; i<vInfo.size(); i++)
        sectionNames.push_back(vInfo[i].head.name);
    }
  }
  return sectionNames;
}

std::string LookaConfigParser::GetString(
  const std::string& secTag,
  const std::string& secName,
  const std::string& secItem,
  const std::string& defaultValue)
{
  std::string r = ReadString(secTag, secName, secItem);
  if (r.empty()) {
    r = defaultValue;
  }
  return r;
}

std::vector<std::string> LookaConfigParser::GetStringV(
  const std::string& secTag,
  const std::string& secName,
  const std::string& secItem)
{
  return ReadStringV(secTag, secName, secItem);
}

int LookaConfigParser::GetInt(
  const std::string& secTag,
  const std::string& secName,
  const std::string& secItem,
  const int& defaultValue)
{
  int r = defaultValue;
  std::string s = ReadString(secTag, secName, secItem);
  if (!s.empty()) {
    r = atoi(s.c_str());
  }
  return r;
}

std::string LookaConfigParser::GetSectionConf(
  const std::string& secTag,
  const std::string& secName)
{
  std::vector<std::string> vSecTags = GetSectionTags();
  std::vector<std::string>::iterator itT = find(vSecTags.begin(), vSecTags.end(), secTag);
  if (itT == vSecTags.end())
    _ERROR_EXIT(-1, "[cannot find section tag:%s]", secTag.c_str());

  std::vector<std::string> vSecNames = GetSectionNames(secTag);
  std::vector<std::string>::iterator itN = find(vSecNames.begin(), vSecNames.end(), secName);
  if (itN == vSecNames.end())
    _ERROR_EXIT(-1, "[cannot find section tag:%s with name:%s]", secTag.c_str(), secName.c_str());
  
  std::string conf;
  std::vector<SectionInfo>& vInfo = m_sec_info[secTag];
  for (unsigned int i=0; i<vInfo.size(); i++) {
    if (vInfo[i].head.name != secName)
      continue;
    conf =  vInfo[i].conf;
    break;
  }
  return conf;
}

std::string LookaConfigParser::ReadString(
  const std::string& secTag,
  const std::string& secName,
  const std::string& secItem)
{
  std::stringstream ss(GetSectionConf(secTag, secName));
  bool findItem = false;
  std::string line;
  std::string curItem;
  std::string curValue;
  while (getline(ss, line)) {
    if (!AnalyseItem(line, curItem, curValue))
      continue;
    if (ToLower(secItem) == ToLower(curItem)) {
      findItem = true;
      break;
    }
  }
  if (!findItem)
    curValue = "";
  return curValue;
}

std::vector<std::string> LookaConfigParser::ReadStringV(
  const std::string& secTag,
  const std::string& secName,
  const std::string& secItem)
{
  std::vector<std::string> values;
  std::stringstream ss(GetSectionConf(secTag, secName));
  std::string line;
  std::string curItem;
  std::string curValue;
  while (getline(ss, line)) {
    if (!AnalyseItem(line, curItem, curValue))
      continue;
    if (ToLower(secItem) == ToLower(curItem))
      values.push_back(curValue);
  }
  return values;
}

int LookaConfigParser::ParsePureConfig(const std::string& pureString)
{
  int s = 0;
  int e = 0;
  std::string secString = "";
  for (unsigned int i=0; i<pureString.length(); i++) {
    if (pureString[i] == '}' /* || i == pureString.length()-1 */) {
      e = i;
      secString = pureString.substr(s, e - s + 1);
      Trim(secString);
      if (ParseSectionConfig(secString))
        _ERROR_RETURN(-1, "[parse section failed]");
      s = i + 1;
    }
  }
  return 0;
}

int LookaConfigParser::ParseSectionConfig(const std::string& secString)
{
  int open_brace  = secString.find('{');
  int close_brace = secString.find('}');
  if (open_brace == -1 ||
      close_brace != (int)secString.length()-1 ||
      std::count(secString.begin(), secString.end(), '{') != 1 ||
      std::count(secString.begin(), secString.end(), '}') != 1)
    _ERROR_RETURN(-1, "[invalid format for section]");
  
  std::string secHeadString = secString.substr(0, open_brace);
  if (secHeadString.empty())
    _ERROR_RETURN(-1, "[must have section head]");

  std::string secTag;
  struct SectionHead secHead;
  if (ParseSectionHead(secHeadString, secTag, secHead.name, secHead.parent))
    _ERROR_RETURN(-1, "[parse section head failed]");
  
  std::string secConfString = secString.substr(open_brace+1, close_brace-open_brace-1);
  if (secConfString.empty())
    _ERROR_RETURN(-1, "[must have section config for %s]", secTag.c_str());

  struct SectionInfo secInfo;
  secInfo.head = secHead;
  secInfo.conf = secConfString;

  std::map<std::string, std::vector<SectionInfo> >::iterator it;
  if ((it = m_sec_info.find(secTag)) != m_sec_info.end()) {
    std::vector<SectionInfo>& vInfo = it->second;
    for (unsigned int i=0; i<vInfo.size(); i++) {
      if (vInfo[i].head.name == secInfo.head.name) {
        _ERROR_RETURN(-1, "[duplicate definition section %s:%s]",
          secTag.c_str(), secInfo.head.name.c_str());
      }
    }
    if (!secInfo.head.parent.empty()) {
      bool findParent = false;
      for (unsigned int i=0; i<vInfo.size(); i++) {
        if (vInfo[i].head.name == secInfo.head.parent) {
          findParent = true;
          secInfo.conf = secInfo.conf + vInfo[i].conf;
        }
      }
      if (!findParent)
        _ERROR_RETURN(-1, "[cannot find parent section %s for %s in %s]",
          secInfo.head.parent.c_str(),
          secInfo.head.name.c_str(),
          secTag.c_str());
    }
    vInfo.push_back(secInfo);
  }
  else {
    std::vector<SectionInfo> vInfo;
    vInfo.push_back(secInfo);
    m_sec_info.insert(std::make_pair(secTag, vInfo));
  }
  return 0;
}

int LookaConfigParser::ParseSectionHead(
  const std::string& secHead,
  std::string& secTag,
  std::string& secName,
  std::string& secParent)
{
  std::string sTag;
  std::string sName;
  std::string sParent;
  std::string sHeadInfo;
  
  int pos = 0;
  while (pos < (int)(secHead.length())) {
    if (IsSpace(secHead[pos]))
      break;
    pos++;
  }

  sTag      = secHead.substr(0, pos);
  sHeadInfo = secHead.substr(pos + 1);

  Trim(sTag);
  if (sTag.empty())
    return -1;

  Trim(sHeadInfo);
  if (!sHeadInfo.empty()) {
    if ((pos = sHeadInfo.find(':')) != -1) {
      sName   = sHeadInfo.substr(0, pos);
      sParent = sHeadInfo.substr(pos+1);
      Trim(sName);
      Trim(sParent);
      if (sName.empty() || sParent.empty())
        _ERROR_RETURN(-1, "[invalid empty name or parent]");
    } else {
      sName = sHeadInfo;
      Trim(sName);
    }
  }

  for (unsigned int i=0; i<sName.length(); i++)
    if (IsSpace(sName[i]))
      _ERROR_RETURN(-1, "[invalid name(%s) for section(%s)]", sName.c_str(), sTag.c_str());
  for (unsigned int i=0; i<sParent.length(); i++)
    if (IsSpace(sParent[i]))
      _ERROR_RETURN(-1, "[invalid parent(%s) for section(%s)]", sParent.c_str(), sTag.c_str());

  secTag = sTag;
  secName = sName;
  secParent = sParent;
  return 0;
}

bool LookaConfigParser::AnalyseItem(const std::string& line, std::string& item, std::string& value)
{
  if (line.empty())
    return false;

  std::string new_line = line;
  DelComment(new_line);
  Trim(new_line);

  int pos;
  if ((pos = new_line.find('=')) == -1)
    return false;

  item  = new_line.substr(0, pos);
  value = new_line.substr(pos + 1);

  Trim(item);
  RemoveEmphasize(item);
  if (item.empty()) {
    return false;
  }
  Trim(value);
  RemoveEmphasize(value);

  return true;
}

bool LookaConfigParser::IsEmphasize(const char& c)
{
  if (c == '\'' || c == '"')
    return true;
  return false;
}

void LookaConfigParser::RemoveEmphasize(std::string &s)
{
  if (IsEmphasize(s[0]) && IsEmphasize(s[s.length()-1]) && s[0] == s[s.length()-1])
    s = s.substr(1, s.length() - 2);
}

bool LookaConfigParser::IsSpace(const char& c)
{
  if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
    return true;
  return false;
}

void LookaConfigParser::Trim(std::string& s)
{
  if (s.empty())
    return;
  std::string::iterator it;
  for (it=s.begin(); it!=s.end() && IsSpace(*it++););
  s.erase(s.begin(), --it);
  for (it=s.end(); it!=s.begin() && IsSpace(*--it););
  s.erase(++it, s.end());
}

void LookaConfigParser::DelComment(std::string& s)
{
  if (s.empty())
    return;
  int pos  = -1;
  int pos1 = s.find("#");
  int pos2 = s.find("//");
  if (pos1 == -1 && pos2 == -1)
    return;
  else if (pos1 != -1 && pos2 != -1)
    pos = std::min(pos1, pos2);
  else
    pos = std::max(pos1, pos2);
  s = s.substr(0, pos);
}

std::string LookaConfigParser::ToLower(const std::string& s)
{
  std::string os(s);
  transform(os.begin(), os.end(), os.begin(), tolower);  
  return os;
}
