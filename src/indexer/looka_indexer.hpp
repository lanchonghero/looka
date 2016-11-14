#ifndef _LOOKA_INDEXER_HPP
#define _LOOKA_INDEXER_HPP
#include <vector>
#include <string>
#include "../looka_mysql_wrapper.hpp"
#include "../looka_config_index.hpp"
#include "../looka_config_source.hpp"
#include "../looka_segmenter.hpp"
#include "../looka_inverter.hpp"
#include "../looka_types.hpp"

class LookaIndexer
{
public:
  LookaIndexer(LookaConfigSource* source_cfg, LookaConfigIndex* index_cfg);
  virtual ~LookaIndexer();

  int DoIndex();

private:
  MysqlWrapper* CreateMysqlWrapper();
  int CheckFields(
    const std::vector<std::string>& attrs,
    const std::vector<std::string>& fields);
  bool IsInFields(
    const std::string& f,
    const std::vector<std::string>& fields,
    int &index);

  bool ProcessDoc(
    LocalDocID ldocid,
    const std::vector<MysqlField>& doc_fields,
    LookaSegmenter* seg,
    LookaInverter<Token, DocInvert*>* inverter,
    std::vector<DocAttr*>* summary);

private:
  LookaConfigSource* m_source_cfg;
  LookaConfigIndex*  m_index_cfg;
};

#endif //_LOOKA_INDEXER_HPP
