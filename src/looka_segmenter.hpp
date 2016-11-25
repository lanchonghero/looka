#ifndef _LOOKA_SEGMENTER_HPP
#define _LOOKA_SEGMENTER_HPP

#include <UnigramCorpusReader.h>
#include <UnigramDict.h>
#include <SynonymsDict.h>
#include <ThesaurusDict.h>
#include <SegmenterManager.h>
#include <Segmenter.h>

#define BUFF_SIZE 1*1024*1024

const std::string punctuation = "!@#$%^&*()-=_+`~;:\",./<>?[]{}|\\！￥…（）——【】、：；‘“’”，。《》？·……";

struct SegmentToken
{
  std::string str;
  uint32_t pos;
};

class LookaSegmenter
{
public:
  LookaSegmenter();
  virtual ~LookaSegmenter();

  bool Init(const std::string& dict_path);
  bool Segment(const std::string& str, std::vector<SegmentToken>& tokens);

private:
  void Trim(std::string& s);
  bool IsSpace(const char& c);
  bool IsPunctuation(const std::string& token);

private:
  bool mInit;
  css::Segmenter* mSegmenter;
  css::SegmenterManager* mSegmenterMgr;
  char mBuffer[BUFF_SIZE];
};

#endif //_LOOKA_SEGMENTER_HPP
