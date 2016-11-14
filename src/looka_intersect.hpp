#ifndef _LOOKA_INTERSECT_HPP
#define _LOOKA_INTERSECT_HPP
#include "looka_log.hpp"
#include "looka_types.hpp"
#include "looka_inverter.hpp"

class TokenIntersect {
public:
  TokenIntersect();
  virtual ~TokenIntersect();

  void SetDocs(std::vector<DocInvert*>* _docs);
  LocalDocID Seek(LocalDocID id);

private:
  std::vector<DocInvert*>* docs;
  unsigned int idx;
};

class LookaIntersect
{
public:
  LookaIntersect();
  virtual ~LookaIntersect();

  void SetTokens(
    const std::vector<std::string>& tokens,
    LookaInverter<Token, DocInvert*>* inverter);
  
  std::vector<DocInvert*>* GetTokenDocs(
    std::string token,
    LookaInverter<Token, DocInvert*>* inverter);

  LocalDocID Seek(LocalDocID id);

private:
  TokenIntersect* tokenInt;
  int size;
};

#endif //_LOOKA_INTERSECT_HPP
