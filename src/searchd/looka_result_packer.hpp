#ifndef _LOOKA_RESULT_PACKER_HPP
#define _LOOKA_RESULT_PACKER_HPP

#include <vector>
#include <string>
#include "../looka_log.hpp"
#include "../looka_string_utils.hpp"
#include "../looka_types.hpp"
#include "../looka_config_source.hpp"
#include "../looka_inverter.hpp"
#include "../looka_intersect.hpp"

class LookaResultPacker
{
public:
  LookaResultPacker() {}
  virtual ~LookaResultPacker() {}
  
  virtual std::string PackResult(
    const LookaConfigSource* source,
    const std::string& query,
    const std::vector<std::string> strtokens,
    const std::vector<DocAttr*>& docs,
    const std::map<DocAttrType, AttrNames*>* attrnames,
    const LookaIntersect* intersect,
    const LookaInverter<Token, DocInvert*>* inverter,
    const std::vector<std::pair<std::string, std::string> >& extra,
    int&  wastetime_us);

  virtual std::string PackResultInternal(
    const LookaConfigSource* source,
    const std::vector<std::pair<std::string, std::string> >& summary,
    const std::vector<DocAttr*>& docs,
    const std::map<DocAttrType, AttrNames*>* attrnames,
    int&  wastetime_us) = 0;

protected:
  bool GetAttrNameIndex(
    const std::map<DocAttrType, AttrNames*>* attrnames,
    const std::string& s, DocAttrType& type, int& index);
};

class LookaResultBasicPacker: public LookaResultPacker
{
public:
  LookaResultBasicPacker() {}
  virtual ~LookaResultBasicPacker() {}

  virtual std::string PackResultInternal(
    const LookaConfigSource* source,
    const std::vector<std::pair<std::string, std::string> >& summary,
    const std::vector<DocAttr*>& docs,
    const std::map<DocAttrType, AttrNames*>* attrnames,
    int&  wastetime_us)
  {
    return "";
  }
};

class LookaResultJsonPacker: public LookaResultPacker
{
public:
  LookaResultJsonPacker() {}
  virtual ~LookaResultJsonPacker() {}

  virtual std::string PackResultInternal(
    const LookaConfigSource* source,
    const std::vector<std::pair<std::string, std::string> >& summary,
    const std::vector<DocAttr*>& docs,
    const std::map<DocAttrType, AttrNames*>* attrnames,
    int&  wastetime_us);
};

class LookaResultXmlPacker: public LookaResultPacker
{
public:
  LookaResultXmlPacker() {}
  virtual ~LookaResultXmlPacker() {}

  virtual std::string PackResultInternal(
    const LookaConfigSource* source,
    const std::vector<std::pair<std::string, std::string> >& summary,
    const std::vector<DocAttr*>& docs,
    const std::map<DocAttrType, AttrNames*>* attrnames,
    int&  wastetime_us);
};

class LookaResultPackerWrapper
{
public:
  LookaResultPackerWrapper();
  virtual ~LookaResultPackerWrapper();
  
  LookaResultPacker* GetResultPacker(const std::string& format);

private:
  LookaResultBasicPacker* m_basic_packer;
  LookaResultJsonPacker*  m_json_packer;
  LookaResultXmlPacker*   m_xml_packer;
};

#endif //_LOOKA_RESULT_PACKER_HPP
