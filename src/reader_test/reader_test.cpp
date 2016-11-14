#include <string>
#include <vector>
#include "../looka_log.hpp"
#include "../looka_file.hpp"
#include "../looka_types.hpp"
#include "../looka_inverter.hpp"
#include "../looka_segmenter.hpp"

class TokenIntersect
{
public:
  TokenIntersect(): docs(NULL), idx(-1) {}
  virtual ~TokenIntersect() {}

  LocalDocID Seek(LocalDocID id)
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

  void SetDocs(std::vector<DocInvert*>* _docs)
  {
    docs = _docs;
    idx = -1;
    _INFO("docs->size() = %d", (int)docs->size());
  }
private:
  std::vector<DocInvert*>* docs;
  unsigned int idx;
};

class Intersect
{
public:
  Intersect():tokenInt(NULL), size(0) {}
  virtual ~Intersect() {if (tokenInt) delete [] tokenInt;}

  LocalDocID Seek(LocalDocID id)
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

  std::vector<DocInvert*>* GetTokenDocs(std::string token, LookaInverter<Token, DocInvert*>* inverter)
  {
    if (!inverter)
      return NULL;
    Token t(token);
    std::vector<DocInvert*>* docs = NULL;
    inverter->GetItems(t, docs);
    return docs;
  }

  void SetTokens(const std::vector<std::string>& tokens, LookaInverter<Token, DocInvert*>* inverter)
  {
    if (tokenInt)
      delete tokenInt;
    size = (int)tokens.size();
    tokenInt = new TokenIntersect[size];
    for (int i=0; i<size; i++)
      tokenInt[i].SetDocs(GetTokenDocs(tokens[i], inverter));
  }

private:
  TokenIntersect* tokenInt;
  int size;
};

int main(int argc, char** argv)
{
  std::vector<DocAttr*>* summary = new std::vector<DocAttr*>();
  LookaIndexReader* reader = new LookaIndexReader();
  LookaInverter<Token, DocInvert*>* inverter =
    new LookaInverter<Token, DocInvert*>();

  std::string index_file = "./data/service/looka_idx_1.lci";
  reader->ReadIndexFromFile(index_file, inverter);

  std::string uint_attr_file   = "./data/service/looka_idx_1.lcu";
  std::string float_attr_file  = "./data/service/looka_idx_1.lcf";
  std::string multi_attr_file  = "./data/service/looka_idx_1.lcm";
  std::string string_attr_file = "./data/service/looka_idx_1.lcs";
  reader->ReadSummaryFromFile(
    uint_attr_file,
    float_attr_file,
    multi_attr_file,
    string_attr_file,
    summary);

  // test query
  std::string query = "亚冠";
  
  // query segment
  std::string dict_path = "/usr/local/mmseg/etc/";
  LookaSegmenter* seg = new LookaSegmenter();
  if (!seg->Init(dict_path)) {
    delete seg;
    _ERROR_RETURN(-1, "init segmenter failed");
  }
  std::vector<std::string> strtokens;
  std::vector<SegmentToken> segtokens;
  seg->Segment(query, segtokens);
  for (size_t i=0; strtokens.size()<segtokens.size(); strtokens.push_back(segtokens[i++].str));

  LocalDocID id = 0;
  Intersect* inter = new Intersect();
  inter->SetTokens(strtokens, inverter);

  struct timeval now;
  gettimeofday(&now, NULL);
  while ((id = inter->Seek(id)) != kIllegalLocalDocID) {
    if (id < summary->size()) {
      _INFO("=======================================");
      _INFO("find doc:%u", id);
      DocAttr* attr = (*summary)[id];
      AttrUint*  u = attr->u;
      AttrFloat* f = attr->f;
      AttrMulti* m = attr->m;
      AttrString* s = attr->s;

      // FilterDoc(attr, query);
      for (uint32_t i=0; i<u->size; i++)
        _INFO("uint_attrs:%u", u->data[i]);
      for (uint32_t i=0; i<m->size; i++)
        _INFO("multi_attrs:%u", m->data[i]);
      for (uint32_t i=0; i<f->size; i++)
        _INFO("float_attrs:%f", f->data[i]);
      for (uint32_t i=0; i<s->size; i++)
        _INFO("string_attrs:%s", s->GetString(i).c_str());
    }
    id++;
  }
  _INFO("cost:%dus", WASTE_TIME_US(now));

  delete inter;
  delete seg;
  delete reader;
  delete inverter;
  return 0;
}
