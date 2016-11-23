// #include <jemalloc/jemalloc.h>
#include "looka_indexer.hpp"
#include "../looka_file.hpp"
#include "../looka_log.hpp"
#include "../looka_str2id.hpp"
#include "../looka_string_utils.hpp"

LookaIndexer::LookaIndexer(LookaConfigSource* source_cfg, LookaConfigIndex* index_cfg):
  m_source_cfg(source_cfg), m_index_cfg(index_cfg)
{
}

LookaIndexer::~LookaIndexer()
{
}

int LookaIndexer::DoIndex()
{
  LookaSegmenter* seg = new LookaSegmenter();
  if (!seg->Init(m_index_cfg->dict_path)) {
    delete seg;
    _ERROR_RETURN(-1, "init segmenter failed");
  }

  MysqlWrapper* mysql = CreateMysqlWrapper();
  if (!mysql)
    _ERROR_RETURN(-1, "create mysql failed");
  if (!mysql->SqlConnect()) {
    delete mysql;
    _ERROR_RETURN(-1, "connect failed");
  }
  if (!mysql->SqlQuery()) {
    delete mysql;
    _ERROR_RETURN(-1, "[sql-failed] [errstr %s]", mysql->SqlError().c_str());
  }

  LookaInverter<Token, DocInvert*>* inverter =
    new LookaInverter<Token, DocInvert*>();
  if (!inverter)
    _ERROR_RETURN(-1, "create inverter failed");
  
  std::vector<DocAttr*>* summary = new std::vector<DocAttr*>();
  if (!summary)
    _ERROR_RETURN(-1, "create summary failed");

  LookaIndexWriter* writer = new LookaIndexWriter();
  if (!writer)
    _ERROR_RETURN(-1, "create indexwriter failed");

  AttrNames* uint_names   = CreateAttrNames(m_source_cfg->sql_attr_uint);
  AttrNames* float_names  = CreateAttrNames(m_source_cfg->sql_attr_float);
  AttrNames* multi_names  = CreateAttrNames(m_source_cfg->sql_attr_multi);
  AttrNames* string_names = CreateAttrNames(m_source_cfg->sql_attr_string);

  _INFO("indexing:%s start...", m_index_cfg->mSectionName.c_str());
  {
    timeval start;
    gettimeofday(&start, NULL);

    LocalDocID ldocid = 0;
    std::vector<MysqlField> fs;
    std::vector<std::string> fn;
    while (mysql->SqlFetchRow(fs)) {
      for (unsigned int i=0; fn.size()<fs.size(); i++)
        fn.push_back(fs[i].name);

      if (CheckFields(m_source_cfg->sql_attr_uint, fn)) break;
      if (CheckFields(m_source_cfg->sql_attr_float, fn)) break;
      if (CheckFields(m_source_cfg->sql_attr_multi, fn)) break;
      if (CheckFields(m_source_cfg->sql_attr_string, fn)) break;
      if (CheckFields(m_source_cfg->sql_field_string, fn)) break;

      ProcessDoc(ldocid++, fs, seg, inverter, summary);

      if (ldocid % 1000 == 0) {
        int waste_time = WASTE_TIME_MS(start);
        _INFO("[processed doc %u] [cost %dms]", ldocid, waste_time);
      }
    }

    // write index
    writer->WriteIndexToFile(m_index_cfg->index_file, inverter);
    // write summary
    writer->WriteSummaryToFile(m_index_cfg->summary_file_uint, summary, uint_names, ATTR_TYPE_UINT);
    writer->WriteSummaryToFile(m_index_cfg->summary_file_float, summary, float_names, ATTR_TYPE_FLOAT);
    writer->WriteSummaryToFile(m_index_cfg->summary_file_multi, summary, multi_names, ATTR_TYPE_MULTI);
    writer->WriteSummaryToFile(m_index_cfg->summary_file_string, summary, string_names, ATTR_TYPE_STRING);

    int waste_time = WASTE_TIME_MS(start);
    _INFO("[docnum %d] [cost %dms]", ldocid, waste_time);
  }

  // free memery
  for (unsigned int i=0; i<summary->size(); i++)
    delete (*summary)[i];

  std::vector<Token> tokens;
  inverter->GetKeys(tokens);
  for (unsigned int i=0; i<tokens.size(); i++) {
    Token& t = tokens[i];
    std::vector<DocInvert*>* doclist;
    inverter->GetItems(t, doclist);
    for (unsigned int j=0; j<doclist->size(); j++)
      free((*doclist)[j]);
  }

  delete mysql;
  delete seg;
  delete inverter;
  delete summary;
  delete writer;
  return 0;
}

bool LookaIndexer::ProcessDoc(
  LocalDocID ldocid,
  const std::vector<MysqlField>& doc_fields,
  LookaSegmenter* seg,
  LookaInverter<Token, DocInvert*>* inverter,
  std::vector<DocAttr*>* summary)
{
  if (!seg)
    return false;

  DocAttr* attr = new DocAttr;

  std::vector<uint32_t> uv(m_source_cfg->sql_attr_uint.size());
  std::vector<uint32_t> mv(m_source_cfg->sql_attr_multi.size());
  std::vector<float>    fv(m_source_cfg->sql_attr_float.size());
  std::vector<std::string> sv(m_source_cfg->sql_attr_string.size());
  std::map<Token, LookaSimpleInverter<FieldID, uint8_t>*> tokenHits;
  for (unsigned int i=0; i<doc_fields.size(); i++)
  {
    int field_index;
    const MysqlField& f = doc_fields[i];
    if (IsInFields(f.name, m_source_cfg->sql_attr_uint, field_index))
      uv[field_index] = (uint32_t)atoi(f.value.c_str());
      //uv.push_back((uint32_t)atoi(f.value.c_str()));
    if (IsInFields(f.name, m_source_cfg->sql_attr_float, field_index))
      fv[field_index] = atof(f.value.c_str());
      //fv.push_back(atof(f.value.c_str()));
    if (IsInFields(f.name, m_source_cfg->sql_attr_string, field_index))
      sv[field_index] = f.value;
      //sv.push_back(f.value);

    // todo: fix multi value
    if (IsInFields(f.name, m_source_cfg->sql_attr_multi, field_index)) {
      std::vector<std::string> v;
      splitString(f.value, ',', v);
      for (unsigned int j=0; j<v.size(); j++)
        if (!(trim(v[j])).empty())
          mv.push_back((uint32_t)atoi(v[j].c_str()));
    }

    if (!IsInFields(f.name, m_source_cfg->sql_field_string, field_index))
      continue;
    FieldID field_id = static_cast<FieldID>(field_index);

    // do segment
    std::vector<SegmentToken> segtokens;
    std::string segvalue = f.value;
    if (!seg->Segment(segvalue, segtokens)) continue;

    for (unsigned int j=0; j<segtokens.size(); j++) {
      Token t(segtokens[j].str);
      if (tokenHits.find(t) == tokenHits.end())
        tokenHits.insert(std::make_pair(t, new LookaSimpleInverter<FieldID, uint8_t>()));
      LookaSimpleInverter<FieldID, uint8_t>* &fieldHits = tokenHits[t];
      fieldHits->Add(field_id, segtokens[j].pos);
    }
  }

  std::map<Token, LookaSimpleInverter<FieldID, uint8_t>*>::iterator it;
  for (it = tokenHits.begin(); it != tokenHits.end(); ++it)
  {
    Token t = it->first;
    LookaSimpleInverter<FieldID, uint8_t>* &fieldHits = it->second;

    uint32_t alloc_hit_size = 0;
    std::vector<FieldID> field_id;
    uint32_t field_num = fieldHits->GetKeys(field_id);
    for (uint32_t i=0; i<field_num; i++) {
      std::vector<uint8_t>* pos_v;
      uint32_t count = fieldHits->GetItems(field_id[i], pos_v);
      alloc_hit_size += sizeof(HitPos) + count * sizeof(uint8_t);
    }

    int32_t doc_size = sizeof(DocInvert) + alloc_hit_size;
    DocInvert* invert = (DocInvert*)malloc(doc_size);
    invert->local_id = ldocid;
    invert->hits_size = alloc_hit_size;

    char* ptr = (char*)(invert->hits);
    for (uint32_t i=0; i<field_num; i++)
    {
      if ((ptr - (char*)invert) > doc_size)
        break;
      std::vector<uint8_t>* pos_v;
      uint32_t count = fieldHits->GetItems(field_id[i], pos_v);

      HitPos* hit = (HitPos*)(ptr);
      hit->field = field_id[i];
      hit->count = count;
      for (uint32_t j=0; j<count; j++)
        hit->pos[j] = (*pos_v)[j];
      ptr += sizeof(HitPos) + count * sizeof(uint8_t);
    }

    inverter->Add(t, invert);
    delete fieldHits;
  }

  AttrUint* ua;
  AttrFloat* fa;
  AttrMulti* ma;
  AttrString* sa;

  ua = (AttrUint*)malloc(sizeof(AttrUint) + uv.size() * sizeof(uint32_t));
  fa = (AttrFloat*)malloc(sizeof(AttrFloat) + fv.size() * sizeof(float));
  ma = (AttrMulti*)malloc(sizeof(AttrMulti) + mv.size() * sizeof(uint32_t));

  int s_alloc_size = sizeof(AttrString) + sv.size() * sizeof(uint32_t);
  for (unsigned int i=0; i<sv.size(); i++)
    s_alloc_size += sv[i].length() + 1;
  sa = (AttrString*)malloc(s_alloc_size);

  ua->size = uv.size();
  fa->size = fv.size();
  ma->size = mv.size();
  sa->size = sv.size();

  for (unsigned int i=0; i<uv.size(); i++)
    ua->data[i] = uv[i];
  for (unsigned int i=0; i<fv.size(); i++)
    fa->data[i] = fv[i];
  for (unsigned int i=0; i<mv.size(); i++)
    ma->data[i] = mv[i];

  int cur_pos = 0;
  int offset = sv.size() * sizeof(uint32_t);
  for (unsigned int i=0; i<sv.size(); i++) {
    sa->len[i] = sv[i].length();
    int abs_pos = offset + cur_pos;
    memcpy(sa->data + abs_pos, &(sv[i][0]), sv[i].length());
    cur_pos += sv[i].length() + 1;
  }

  attr->u = ua;
  attr->f = fa;
  attr->m = ma;
  attr->s = sa;

  summary->push_back(attr);

  return true;
}


MysqlWrapper* LookaIndexer::CreateMysqlWrapper()
{
  MysqlParams params;
  params.m_sHost  = m_source_cfg->sql_host;
  params.m_sUser  = m_source_cfg->sql_user;
  params.m_sPass  = m_source_cfg->sql_pass;
  params.m_sDB    = m_source_cfg->sql_db;
  params.m_iPort  = m_source_cfg->sql_port;
  params.m_sUsock = m_source_cfg->sql_socket;
  params.m_bPrintQueries  = m_source_cfg->sql_print_query;
  params.m_vQueryPre      = m_source_cfg->sql_query_pre_set;
  params.m_vQueryPost     = m_source_cfg->sql_query_post_set;
  params.m_sDftTable      = m_source_cfg->sql_table;
  params.m_sQuery         = m_source_cfg->sql_query;
  return new MysqlWrapper(params);
}

int LookaIndexer::CheckFields(
  const std::vector<std::string>& attrs,
  const std::vector<std::string>& fields)
{
  for (unsigned int i=0; i<attrs.size(); i++)
    if (find(fields.begin(), fields.end(), attrs[i]) == fields.end())
      _ERROR_RETURN(-1, "[Unknown column '%s']", attrs[i].c_str());
  return 0;
}

bool LookaIndexer::IsInFields(
  const std::string& f,
  const std::vector<std::string>& fields,
  int& index)
{
  const std::vector<std::string>::const_iterator it =
    find(fields.begin(), fields.end(), f);
  if (it == fields.end()) {
    index = -1;
    return false;
  }
  index = std::distance(fields.begin(), it);
  return true;
}

AttrNames* LookaIndexer::CreateAttrNames(const std::vector<std::string>& attrs)
{
  AttrNames* a = NULL;

  int s_alloc_size = sizeof(AttrNames) + attrs.size() * sizeof(uint32_t);
  for (unsigned int i=0; i<attrs.size(); i++)
    s_alloc_size += attrs[i].length() + 1;
  a = (AttrNames*)malloc(s_alloc_size);
  a->size = attrs.size();
  
  int cur_pos = 0;
  int offset = attrs.size() * sizeof(uint32_t);
  for (unsigned int i=0; i<attrs.size(); i++) {
    a->len[i] = attrs[i].length();
    int abs_pos = offset + cur_pos;
    memcpy(a->data + abs_pos, &(attrs[i][0]), attrs[i].length());
    cur_pos += attrs[i].length() + 1;
  }

  return a;
}
