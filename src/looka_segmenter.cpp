#include "looka_segmenter.hpp"

LookaSegmenter::LookaSegmenter():
  mInit(false)
{
}

LookaSegmenter::~LookaSegmenter()
{
}

bool LookaSegmenter::Init(const std::string& dict_path)
{
  if (!mInit) {
    mSegmenterMgr = new css::SegmenterManager();
    if (mSegmenterMgr->init(dict_path.c_str()) == 0) {
      mSegmenter = mSegmenterMgr->getSegmenter();
      mInit = true;
    } else {
      delete mSegmenterMgr;
      mSegmenterMgr = NULL;
    }
  }
  return mInit;
}

bool LookaSegmenter::Segment(std::string& str, std::vector<SegmentToken>& tokens)
{
  if (!mInit)
    return false;

  if (!mSegmenter)
    return false;

  uint32_t  cur_pos = 0;
  u2 len    = 0;
  u2 symlen = 0;
  mSegmenter->setBuffer((u1*)(str.c_str()), str.length());
  while (true) {
    char* ptr = (char*)mSegmenter->peekToken(len, symlen);
    if (ptr == NULL || len == 0) break;
    mSegmenter->popToken(len);
    std::string tok = std::string(ptr).substr(0, len);
    cur_pos += len;
    Trim(tok);
    if (tok.empty()) continue;
    if (IsPunctuation(tok)) continue;

    SegmentToken segTok;
    segTok.str = tok;
    segTok.pos = cur_pos;
    tokens.push_back(segTok);
  }
  return true;
}

bool LookaSegmenter::IsSpace(const char& c)
{
  if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
    return true;
  return false;
}

bool LookaSegmenter::IsPunctuation(const std::string& token)
{
  return (punctuation.find(token) != std::string::npos);
}

void LookaSegmenter::Trim(std::string& s)
{
  if (s.empty())
    return;
  std::string::iterator it;
  for (it=s.begin(); it!=s.end() && IsSpace(*it++););
  s.erase(s.begin(), --it);
  for (it=s.end(); it!=s.begin() && IsSpace(*--it););
  s.erase(++it, s.end());
}
