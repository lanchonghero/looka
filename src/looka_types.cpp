#include "looka_types.hpp"
#include "looka_str2id.hpp"

DocInvert::DocInvert():
  local_id(kIllegalLocalDocID)/*, tf(0), field_index(-1)*/
{
}

DocInvert::DocInvert(LocalDocID id):
  local_id(id)/*, tf(0), field_index(-1)*/
{
}

DocInvert::DocInvert(const DocInvert& d)
{
  local_id = d.local_id;
  //tf = d.tf;
  //field_index = d.field_index;
}

DocInvert& DocInvert::operator = (const DocInvert& d)
{
  if (!(this == &d)) {
    local_id = d.local_id;
    //tf = d.tf;
    //field_index = d.field_index;
  }
  return *this;
}

bool DocInvert::operator == (const DocInvert& d) const
{
  return local_id == d.local_id;
}

bool DocInvert::operator < (const DocInvert& d) const
{
  return local_id < d.local_id;
}

bool DocInvert::operator > (const DocInvert& d) const
{
  return local_id > d.local_id;
}

Token::Token():
  token_id(kIllegalTokenID),
  token_str("")
{
}

Token::Token(std::string& str):
  token_id(ComputeTokenID(str)),
  token_str(str)
{
}

Token::Token(const Token& t)
{
  token_id  = t.token_id;
  token_str = t.token_str;
}

Token& Token::operator = (const Token& t)
{
  if (!(this == &t)) {
    token_id  = t.token_id;
    token_str = t.token_str;
  }
  return *this;
}

bool Token::operator == (const Token& t) const
{
  return (token_id == t.token_id) && (token_str == t.token_str);
}

bool Token::operator < (const Token& t) const
{
  return (token_id < t.token_id);
}

bool Token::operator > (const Token& t) const
{
  return (token_id > t.token_id);
}

TokenID Token::id()
{
  return token_id;
}

std::string Token::str()
{
  return token_str;
}
