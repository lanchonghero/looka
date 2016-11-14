#include <string.h>
#include <json/json.h>
#include <libxml/parser.h>
#include "looka_result_packer.hpp"

std::string LookaResultPacker::PackResult(
  const LookaConfigSource* source,
  const std::string& query,
  const std::vector<std::string> strtokens,
  const std::vector<DocAttr*>& docs,
  const LookaIntersect* intersect,
  const LookaInverter<Token, DocInvert*>* inverter,
  const std::vector<std::pair<std::string, std::string> >& extra,
  int&  wastetime_us)
{
  std::vector<std::pair<std::string, std::string> > summary;
  summary.push_back(std::make_pair("query", query));
  summary.push_back(std::make_pair("doc_num", intToString(static_cast<int>(docs.size()))));
  std::copy(extra.begin(), extra.end(), std::back_inserter(summary));

  return PackResultInternal(source, summary, docs, wastetime_us);
}

std::string LookaResultJsonPacker::PackResultInternal(
  const LookaConfigSource* source,
  const std::vector<std::pair<std::string, std::string> >& summary,
  const std::vector<DocAttr*>& docs,
  int&  wastetime_us)
{
  struct timeval pack_start;
  gettimeofday(&pack_start, NULL);

  Json::Value root;
  Json::Value jsonDocs;
  Json::Value item;
  for (size_t i=0; i<summary.size(); i++) {
    const std::pair<std::string, std::string>& kvpair = summary[i];
    const std::string& k = kvpair.first;
    const std::string& v = kvpair.second;
    root[k] = v;
  }

  for (size_t i=0; i<docs.size(); i++) {
    DocAttr* const& attr = docs[i];
    for (size_t j=0; j<attr->u->size; j++) {
      item[source->sql_attr_uint[j]] = attr->u->data[j];
    }
    for (size_t j=0; j<attr->s->size; j++) {
      item[source->sql_attr_string[j]] = attr->s->GetString(j);
    }
    jsonDocs.append(item);
  }
  root["docs"] = jsonDocs;

  wastetime_us = WASTE_TIME_US(pack_start);
  std::string pack_cost_str = intToString(wastetime_us) + "us";
  root["pack_cost"] = pack_cost_str;
  return root.toStyledString();
}

std::string LookaResultXmlPacker::PackResultInternal(
  const LookaConfigSource* source,
  const std::vector<std::pair<std::string, std::string> >& summary,
  const std::vector<DocAttr*>& docs,
  int&  wastetime_us)
{
  struct timeval pack_start;
  gettimeofday(&pack_start, NULL);
  
  std::string xmlresult;
  xmlDocPtr  document = xmlNewDoc(BAD_CAST("1.0"));
  xmlNodePtr display  = xmlNewNode(NULL, BAD_CAST("display"));
  xmlDocSetRootElement(document, display);

  for (size_t i=0; i<summary.size(); i++) {
    const std::pair<std::string, std::string>& kvpair = summary[i];
    const std::string& k = kvpair.first;
    const std::string& v = kvpair.second;
    xmlNewTextChild(display, NULL,
      BAD_CAST(const_cast<char*>(k.c_str())),
      BAD_CAST(const_cast<char*>(v.c_str())));
  }

  xmlNodePtr docs_root = xmlNewNode(NULL, BAD_CAST("docs"));
  for (size_t i=0; i<docs.size(); i++) {
    xmlNodePtr item = xmlNewNode(NULL, BAD_CAST("item"));
    DocAttr* const& attr = docs[i];
    std::string tag, val;
    for (size_t j=0; j<attr->s->size; j++) {
      tag = source->sql_attr_string[j];
      val = attr->s->GetString(j);
      xmlNewTextChild(item, NULL,
        BAD_CAST(const_cast<char*>(tag.c_str())),
        BAD_CAST(const_cast<char*>(val.c_str())));
    }
    for (size_t j=0; j<attr->u->size; j++) {
      tag = source->sql_attr_uint[j];
      val = intToString(static_cast<int>(attr->u->data[j]));
      xmlNewTextChild(item, NULL,
        BAD_CAST(const_cast<char*>(tag.c_str())),
        BAD_CAST(const_cast<char*>(val.c_str())));
    }
    xmlAddChild(docs_root, item);
  }
  
  wastetime_us = WASTE_TIME_US(pack_start);
  std::string pack_cost_str = intToString(wastetime_us) + "us";
  xmlNewTextChild(display, NULL,
    BAD_CAST("pack_cost"),
    BAD_CAST(const_cast<char*>(pack_cost_str.c_str())));

  xmlAddChild(display, docs_root);

  xmlChar* doc_text;
  int doc_len;
  xmlDocDumpFormatMemoryEnc(document, &doc_text, &doc_len, "gbk", 1);

  xmlresult = std::string((char*)doc_text, doc_len);
  xmlFree(doc_text);
  
  return xmlresult;
}

LookaResultPackerWrapper::LookaResultPackerWrapper()
{
  m_basic_packer = new LookaResultBasicPacker();
  m_json_packer  = new LookaResultJsonPacker();
  m_xml_packer   = new LookaResultXmlPacker();
}

LookaResultPackerWrapper::~LookaResultPackerWrapper()
{
  delete m_basic_packer;
  delete m_json_packer;
  delete m_xml_packer;
}

LookaResultPacker* LookaResultPackerWrapper::GetResultPacker(const std::string& format)
{
  if (strcasecmp(format.c_str(), "json") == 0) {
    return m_json_packer;
  } else if (strcasecmp(format.c_str(), "xml") == 0) {
    return m_xml_packer;
  }

  return m_basic_packer;
}
