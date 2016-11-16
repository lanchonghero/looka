#ifndef _LOOKA_CONFIG_PARSER_HPP
#define _LOOKA_CONFIG_PARSER_HPP

#include <map>
#include <vector>
#include <string>

class LookaConfigParser
{
public:
  LookaConfigParser();
  virtual ~LookaConfigParser();

  int LoadConfig(const std::string& filename);
  std::vector<std::string> GetSectionTags();
  std::vector<std::string> GetSectionNames(const std::string& secTag);

  std::string GetString(
    const std::string& secTag,
    const std::string& secName,
    const std::string& secItem,
    const std::string& defaultValue);
  
  std::vector<std::string> GetStringV(
    const std::string& secTag,
    const std::string& secName,
    const std::string& secItem);

  int GetInt(
    const std::string& secTag,
    const std::string& secName,
    const std::string& secItem,
    const int& defaultValue);

private:
  std::string ReadString(
    const std::string& secTag,
    const std::string& secName,
    const std::string& secItem);

  std::vector<std::string> ReadStringV(
    const std::string& secTag,
    const std::string& secName,
    const std::string& secItem);

  std::string GetSectionConf(
  const std::string& secTag,
  const std::string& secName);

  int ParsePureConfig(const std::string& pureString);
  int ParseSectionConfig(const std::string& secString);
  int ParseSectionHead(
    const std::string& secHead,
    std::string& secTag,
    std::string& secName,
    std::string& secParent);
  bool AnalyseItem(
    const std::string& line,
    std::string& item,
    std::string& value);

  bool IsEmphasize(const char& c);
  void RemoveEmphasize(std::string& s);
  bool IsSpace(const char& c);
  void Trim(std::string& s);
  void DelComment(std::string& s);
  std::string ToLower(const std::string& s);

private:
  struct SectionHead {
    std::string name;
    std::string parent;
  };
  struct SectionInfo {
    SectionHead head;
    std::string conf;
  };

private:
  std::map<std::string, std::vector<SectionInfo> > m_sec_info;
};

#endif //_LOOKA_CONFIG_PARSER_HPP
