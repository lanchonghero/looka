#include "looka_intersect.hpp"

///////////////////////////////////////////////////////
///////////////////////////////////////////////////////

TokenIntersect::TokenIntersect(): docs(NULL), idx(-1)
{
}

TokenIntersect::~TokenIntersect()
{
}

LocalDocID TokenIntersect::Seek(LocalDocID id)
{
  if (!docs)
    return kIllegalLocalDocID;
  if (docs->empty())
    return kIllegalLocalDocID;
  for (unsigned int i = 0; i < docs->size(); i++) {
    if ((*docs)[i]->local_id >=  id) {
      return (*docs)[i]->local_id;
    }
  }
  return kIllegalLocalDocID;
}
  
void TokenIntersect::SetDocs(std::vector<DocInvert*>* _docs)
{
  docs = _docs;
  idx = -1;
}

///////////////////////////////////////////////////////
///////////////////////////////////////////////////////

LookaIntersect::LookaIntersect(): tokenInt(NULL), size(0)
{
}

LookaIntersect::~LookaIntersect()
{
  if (tokenInt)
    delete []tokenInt;
}


LocalDocID LookaIntersect::Seek(LocalDocID id)
{
  LocalDocID next_id;
  if (!tokenInt || size <= 0)
    return kIllegalLocalDocID;

  id = tokenInt[0].Seek(id);
  if (id == kIllegalLocalDocID)
    return kIllegalLocalDocID;

  for (int i = 1; i < size; i++)
  {
again:
    next_id = tokenInt[i].Seek(id);
    if (next_id != id)
    {
      if (next_id == kIllegalLocalDocID)
        return kIllegalLocalDocID;

      id = tokenInt[0].Seek(next_id);
      if (id == kIllegalLocalDocID)
        return kIllegalLocalDocID;

      i = 1;
      goto again;
    }
  }
  return id;
}

std::vector<DocInvert*>* LookaIntersect::GetTokenDocs(
  std::string token,
  LookaInverter<Token, DocInvert*>* inverter)
{
  if (!inverter)
    return NULL;
  Token t(token);
  std::vector<DocInvert*>* docs = NULL;
  inverter->GetItems(t, docs);
  return docs;
}

void LookaIntersect::SetTokens(
  const std::vector<std::string>& tokens,
  LookaInverter<Token, DocInvert*>* inverter)
{
  if (tokenInt)
    delete tokenInt;
  size = tokens.size();
  tokenInt = new TokenIntersect[size];
  for (int i=0; i<size; i++)
    tokenInt[i].SetDocs(GetTokenDocs(tokens[i], inverter));
}
