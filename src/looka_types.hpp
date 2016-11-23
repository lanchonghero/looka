#ifndef _LOOKA_TYPES_HPP
#define _LOOKA_TYPES_HPP

#include <stdint.h>
#include <string>
#include <vector>
#include <unordered_map>

const uint32_t kuint32max = ((uint32_t) 0xFFFFFFFF);
const uint64_t kuint64max = ((uint64_t) 0xFFFFFFFFFFFFFFFF);

typedef uint64_t GlobalDocID;
static const GlobalDocID kIllegalGlobalDocID = kuint64max;

typedef uint32_t LocalDocID;
static const LocalDocID kIllegalLocalDocID = kuint32max;

typedef uint64_t TokenID;
static const TokenID kIllegalTokenID = kuint64max;

typedef uint8_t FieldID;

enum DocAttrType {
  ATTR_TYPE_UINT = 0,
  ATTR_TYPE_FLOAT,
  ATTR_TYPE_MULTI,
  ATTR_TYPE_STRING,
};

struct AttrUint {
  uint8_t size;
  uint32_t data[];
};

struct AttrFloat {
  uint8_t size;
  float data[];
};

struct AttrMulti {
  uint8_t size;
  uint32_t data[];
};

struct AttrString {
  uint8_t  size;
  union {
    uint32_t len[];
    char     data[];
  };

  std::string GetString(int idx) {
    std::string s("");
    if (idx >= 0 && size > 0) {
      uint32_t pos = size * sizeof(uint32_t);
      for (int i=0; i<idx; i++)
        pos += len[i] + 1;
      s = std::string(data+pos, len[idx]);
    }
    return s;
  }
};

typedef struct AttrString  AttrNames;

struct DocAttr {
  AttrUint   *u;
  AttrFloat  *f;
  AttrMulti  *m;
  AttrString *s;
};
 
struct HitPos {
  FieldID field;
  uint8_t count;
  uint8_t pos[];
};

class DocInvert
{
public:
  DocInvert();
  DocInvert(LocalDocID id);
  DocInvert(const DocInvert& d);
  DocInvert& operator = (const DocInvert& d);
  bool operator == (const DocInvert& d) const;
  bool operator < (const DocInvert& d) const;
  bool operator > (const DocInvert& d) const;

  void SetLocalDocID(LocalDocID id) {local_id = id;}
  LocalDocID GetLocalDocID() {return local_id;}
  //void SetTF(uint8_t _tf) {tf = _tf;}
  //uint8_t GetTF() {return tf;}
  //void SetFieldIndex(uint8_t idx) {field_index = idx;}
  //uint8_t GetFieldIndex() {return field_index;}

  void GetHits(std::vector<HitPos*>& h)
  {
    int   cur_size = 0;
    while (cur_size < hits_size) {
      HitPos* hit = (HitPos*)((char*)hits + cur_size);
      cur_size += sizeof(HitPos) + hit->count * sizeof(uint8_t);
      h.push_back(hit);
    }
  }

public:
  struct Hash
  {
    size_t operator() (const DocInvert& d) const
    {
      DocInvert* p = const_cast<DocInvert*>(&d);
      return std::hash<LocalDocID>()(p->GetLocalDocID());
    }
  };

public:
  LocalDocID local_id;
  uint8_t hits_size;
  HitPos hits[];
};


class Token
{
public:
  Token();
  Token(std::string& str);
  Token(const Token& t);
  Token& operator = (const Token& t);
  bool operator == (const Token& t) const;
  bool operator < (const Token& t) const;
  bool operator > (const Token& t) const;

  TokenID id();
  std::string str();

public:
  struct Hash
  {
    size_t operator() (const Token& t) const
    {
      Token* p = const_cast<Token*>(&t);
      return std::hash<TokenID>()(p->id());
    }
  };

private:
  TokenID token_id;
  std::string token_str;
};

#endif //_LOOKA_TYPES_HPP
