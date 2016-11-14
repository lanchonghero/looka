#ifndef _LOOKA_FILE_HPP
#define _LOOKA_FILE_HPP

#include "looka_inverter.hpp"
#include "looka_types.hpp"

class LookaIndexReader
{
public:
  LookaIndexReader();
  virtual ~LookaIndexReader();

  bool ReadIndexFromFile(
    std::string& index_file,
    LookaInverter<Token, DocInvert*>*& inverte);

  bool ReadSummaryFromFile(
    std::string& uint_attr_file,
    std::string& float_attr_file,
    std::string& multi_attr_file,
    std::string& string_attr_file,
    std::vector<DocAttr*>*& summary);
};

class LookaIndexWriter
{
public:
  LookaIndexWriter();
  virtual ~LookaIndexWriter();

  bool WriteIndexToFile(
    std::string& index_file,
    LookaInverter<Token, DocInvert*>*& inverter);

  bool WriteSummaryToFile(
    std::string& summary_file,
    std::vector<DocAttr*>*& docs,
    DocAttrType type);
};

#endif //_LOOKA_FILE_HPP
