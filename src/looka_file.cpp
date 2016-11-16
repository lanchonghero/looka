#include <fstream>
#include <vector>
#include <string.h>
#include "looka_file.hpp"
#include "looka_log.hpp"

#define BUFF_SIZE 100

LookaIndexReader::LookaIndexReader()
{
}

LookaIndexReader::~LookaIndexReader()
{
}

bool LookaIndexReader::ReadIndexFromFile(
  std::string& index_file,
  LookaInverter<Token, DocInvert*>*& inverter)
{
  if (!inverter)
    return false;

  std::ifstream f(index_file.c_str(), std::ios::binary);
  if (!f) {
    _ERROR("[cannot open file %s]", index_file.c_str());
    return false;
  }

  int file_size = 0;
  int curr_size = 0;
  char buf[BUFF_SIZE];

  f.seekg (0, f.end);
  file_size = f.tellg();
  f.seekg (0, f.beg);

  while (curr_size < file_size)
  {
    TokenID id;
    f.read((char*)&id, sizeof(id));
    curr_size += sizeof(id);

    uint32_t len;
    f.read((char*)&len, sizeof(len));
    curr_size += sizeof(len);

    memset(buf, 0, sizeof(buf));
    f.read(buf, len);
    curr_size += len;

    std::string str(buf);
    Token t(str);

    uint32_t docinvert_count;
    f.read((char*)&docinvert_count, sizeof(docinvert_count));
    curr_size += sizeof(docinvert_count);

    std::string log;
    char log_buf[1024];
    for (uint32_t i=0; i<docinvert_count; i++) {
      DocInvert temp_doc;
      f.read((char*)&temp_doc, sizeof(DocInvert));
      curr_size += sizeof(DocInvert);

      DocInvert* doc = (DocInvert*)malloc(sizeof(DocInvert) + temp_doc.hits_size);
      memcpy(doc, &temp_doc, sizeof(DocInvert));
      f.read((char*)((char*)doc + sizeof(DocInvert)), doc->hits_size);
      curr_size += doc->hits_size;

      inverter->Add(t, doc);

      /*
      // log
      sprintf(log_buf, "id:%u ", doc->local_id, doc->hits_size);
      log += std::string(log_buf);

      HitPos *hit = doc->hits;

      int cur_hit_size = 0;
      while (cur_hit_size < doc->hits_size) {
        HitPos* hit = (HitPos*)((char*)(doc->hits) + cur_hit_size);
        cur_hit_size += sizeof(HitPos) + hit->count * sizeof(uint8_t);
        char buf[64];
        sprintf(buf, "field:%u count:%u ", hit->field, hit->count);
        log += std::string(buf);
      }

      if (i != docinvert_count - 1)
        log += "\t";
        */
    }
    //_INFO("doclist->%s", log.c_str());
  }

  f.close();
  return true;
}

bool LookaIndexReader::ReadSummaryFromFile(
  std::string& uint_attr_file,
  std::string& float_attr_file,
  std::string& multi_attr_file,
  std::string& string_attr_file,
  std::vector<DocAttr*>*& summary)
{
  if (!summary)
    return false;

  std::ifstream f_u(uint_attr_file.c_str(), std::ios::binary);
  if (!f_u) {
    _ERROR("[cannot open file %s]", uint_attr_file.c_str());
    return false;
  }
  std::ifstream f_f(float_attr_file.c_str(), std::ios::binary);
  if (!f_f) {
    _ERROR("[cannot open file %s]", float_attr_file.c_str());
    return false;
  }
  std::ifstream f_m(multi_attr_file.c_str(), std::ios::binary);
  if (!f_m) {
    _ERROR("[cannot open file %s]", multi_attr_file.c_str());
    return false;
  }
  std::ifstream f_s(string_attr_file.c_str(), std::ios::binary);
  if (!f_s) {
    _ERROR("[cannot open file %s]", string_attr_file.c_str());
    return false;
  }


  uint32_t count;
  uint32_t temp;

  f_u.read((char*)&temp, sizeof(temp));
  count = temp;

  f_f.read((char*)&temp, sizeof(temp));
  if (count != temp) {
    _ERROR("[read count error]");
    return false;
  }

  f_m.read((char*)&temp, sizeof(temp));
  if (count != temp) {
    _ERROR("[read count error]");
    return false;
  }
  
  f_s.read((char*)&temp, sizeof(temp));
  if (count != temp) {
    _ERROR("[read count error]");
    return false;
  }

  uint32_t i = 0;
  summary->resize(count);
  while (i < count) {
    DocAttr* attr = new DocAttr();
    uint8_t size;

    f_u.read((char*)&size, sizeof(size));
    AttrUint* p_u = (AttrUint*)malloc(sizeof(AttrUint) + sizeof(uint32_t) * size);
    f_u.read((char*)(p_u->data), sizeof(uint32_t) * size);
    p_u->size = size;
    attr->u = p_u;
    
    f_f.read((char*)&size, sizeof(size));
    AttrFloat* p_f = (AttrFloat*)malloc(sizeof(AttrFloat) + sizeof(float) * size);
    f_f.read((char*)(p_f->data), sizeof(float) * size);
    p_f->size = size;
    attr->f = p_f;
    
    f_m.read((char*)&size, sizeof(size));
    AttrMulti* p_m = (AttrMulti*)malloc(sizeof(AttrMulti) + sizeof(uint32_t) * size);
    f_m.read((char*)(p_m->data), sizeof(uint32_t) * size);
    p_m->size = size;
    attr->m = p_m;

    f_s.read((char*)&size, sizeof(size));
    int len_array_size  = sizeof(uint32_t) * size;
    uint32_t* len_array = (uint32_t*)malloc(len_array_size);
    f_s.read((char*)len_array, len_array_size);
    int strings_size = 0;
    for (uint32_t j=0; j<size; j++)
      strings_size += len_array[j] + 1;

    AttrString* p_s = (AttrString*)malloc(sizeof(AttrString) + len_array_size + strings_size);
    memcpy((char*)(p_s->len), (char*)len_array, len_array_size);
    f_s.read((char*)p_s->data + len_array_size, strings_size);
    p_s->size = size;
    attr->s = p_s;

    /*
    for (uint32_t j=0; j<size; j++)
      _INFO("string[%u]:%s", j, p_s->GetString(j).c_str());
      */
    free(len_array);

    (*summary)[i] = attr;
    i++;
  }

  f_u.close();
  f_f.close();
  f_m.close();
  f_s.close();
  return true;
}

LookaIndexWriter::LookaIndexWriter()
{
}

LookaIndexWriter::~LookaIndexWriter()
{
}

bool LookaIndexWriter::WriteIndexToFile(
  std::string& index_file,
  LookaInverter<Token, DocInvert*>*& inverter)
{
  if (!inverter)
    return false;

  std::ofstream f(index_file.c_str(), std::ios::binary);
  if (!f) {
    _ERROR("[cannot open file %s]", index_file.c_str());
    return false;
  }
  
  std::vector<Token> tokens;
  uint32_t token_count = inverter->GetKeys(tokens);
  for (uint32_t i=0; i<token_count; i++)
  {
    // write token
    Token& t = tokens[i];
    TokenID id = t.id();
    f.write((char*)&id, sizeof(id));
    uint32_t length = t.str().length();
    f.write((char*)&length, sizeof(length));
    f.write(t.str().c_str(), length);

    // write invert
    std::vector<DocInvert*>* docinvert;
    uint32_t docinvert_count = inverter->GetItems(t, docinvert);

    f.write((char*)&docinvert_count, sizeof(docinvert_count));
    for (uint32_t j=0; j<docinvert_count; j++) {
      DocInvert* invert = (*docinvert)[j];
      f.write((char*)invert, sizeof(DocInvert) + invert->hits_size);
    }
  }

  f.close();
  return true;
}

bool LookaIndexWriter::WriteSummaryToFile(
  std::string& summary_file,
  std::vector<DocAttr*>*& docs,
  DocAttrType type)
{
  std::ofstream file(summary_file.c_str(), std::ios::binary);
  if (!file) {
    _ERROR("[cannot open file %s]", summary_file.c_str());
    return false;
  }

  uint32_t count = docs->size();
  file.write((char*)&count, sizeof(count));

  if (type == ATTR_TYPE_UINT) {
    for (unsigned int i=0; i<docs->size(); i++) {
      AttrUint*& u = (*docs)[i]->u;
      file.write((char*)&(u->size), sizeof(u->size));
      file.write((char*)(u->data),  u->size * sizeof(uint32_t));
    }
  } else if (type == ATTR_TYPE_FLOAT) {
    for (unsigned int i=0; i<docs->size(); i++) {
      AttrFloat*& f = (*docs)[i]->f;
      file.write((char*)&(f->size), sizeof(f->size));
      file.write((char*)(f->data),  f->size * sizeof(float));
    }
  } else if (type == ATTR_TYPE_MULTI) {
    for (unsigned int i=0; i<docs->size(); i++) {
      AttrMulti*& m = (*docs)[i]->m;
      file.write((char*)&(m->size), sizeof(m->size));
      file.write((char*)(m->data),  m->size * sizeof(uint32_t));
    }
  } else if (type == ATTR_TYPE_STRING) {
    for (unsigned int i=0; i<docs->size(); i++) {
      AttrString*& s = (*docs)[i]->s;
      file.write((char*)&(s->size), sizeof(s->size));
      file.write((char*)(s->len),  s->size * sizeof(uint32_t));
      int data_size = 0;
      for (unsigned int j=0; j<s->size; j++)
        data_size += s->len[j] + 1;
      file.write((char*)(s->data + s->size*sizeof(uint32_t)), data_size);
      /*
      for (unsigned int j=0; j<s->size; j++)
        _INFO("string[%u]:%s", j, s->GetString(j).c_str());
        */
    }
  }

  file.close();
  return true;
}
