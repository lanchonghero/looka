#include <iostream>
#include <string>
#include <pthread.h>
#include <signal.h>
#include <libxml/parser.h>
#include "../looka_file.hpp"
#include "../looka_intersect.hpp"
#include "looka_searchd.hpp"

LookaSearchd::LookaSearchd(
  LookaConfigSource* source_cfg,
  LookaConfigIndex* index_cfg,
  LookaConfigSearchd* searchd_cfg)
{
  m_source_cfg = source_cfg;
  m_index_cfg = index_cfg;
  m_searchd_cfg = searchd_cfg;
  // m_role = searchd_cfg->role;
  m_role = searchd_cfg->role;

  if (!m_source_cfg)
    _ERROR_EXIT(-1, "invalid source config");
  if (!m_index_cfg)
    _ERROR_EXIT(-1, "invalid index config");
  if (!m_searchd_cfg)
    _ERROR_EXIT(-1, "invalid searchd config");

  m_segmenter = new LookaSegmenter();
  if (!m_segmenter->Init(m_index_cfg->dict_path)) {
    delete m_segmenter;
    _ERROR_EXIT(-1, "init segmenter failed");
  }

  m_inverter = new LookaInverter<Token, DocInvert*>();
  m_summary  = new std::vector<DocAttr*>();
  m_result_packer_wrapper = new LookaResultPackerWrapper();
  m_attr_names = new std::map<DocAttrType, AttrNames*>();

  (*m_attr_names)[ATTR_TYPE_UINT]   = NULL;
  (*m_attr_names)[ATTR_TYPE_FLOAT]  = NULL;
  (*m_attr_names)[ATTR_TYPE_MULTI]  = NULL;
  (*m_attr_names)[ATTR_TYPE_STRING] = NULL;

  m_client = NULL;
  m_server = NULL;
  if (m_role == "parent") {
    m_client = new tcp_client<LookaSearchd, LookaSearchd>();
    m_client->register_send_handler(this, &LookaSearchd::TcpClientSendCallback);
    m_client->register_recv_handler(this, &LookaSearchd::TcpClientRecvHandleData);
  } else if (m_role == "leaf") {
    m_server = new tcp_server<LookaSearchd, LookaSearchd>();
    m_server->register_send_handler(this, &LookaSearchd::TcpServerSendCallback);
    m_server->register_recv_handler(this, &LookaSearchd::TcpServerRecvHandleData);
  }
  pthread_mutex_init(&m_seg_lock, NULL);
}

LookaSearchd::~LookaSearchd()
{
  if (m_segmenter)
    delete m_segmenter;
  if (m_inverter)
    delete m_inverter;
  if (m_summary)
    delete m_summary;
  if (m_result_packer_wrapper)
    delete m_result_packer_wrapper;
  if (m_attr_names)
    delete m_attr_names;
  if (m_client)
    delete m_client;
  if (m_server)
    delete m_server;
  pthread_mutex_destroy(&m_seg_lock);
}

bool LookaSearchd::Init()
{
  if (m_role == "leaf") {
    _INFO("[reading index & summary ...]");
    LookaIndexReader* reader = new LookaIndexReader();

    reader->ReadIndexFromFile(m_index_cfg->index_file, m_inverter);
    reader->ReadSummaryFromFile(
      m_index_cfg->summary_file_uint,
      m_index_cfg->summary_file_float,
      m_index_cfg->summary_file_multi,
      m_index_cfg->summary_file_string,
      m_attr_names,
      m_summary);
    delete reader;

    m_server->net(6610);
    m_server->start();
  } else if (m_role == "parent") {
    std::vector<struct sockaddr_in> addrs;
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    //addr.sin_port = htons(33521);
    addr.sin_port = htons(6610);
    addrs.push_back(addr);

    m_client->net(&addrs[0], addrs.size(), 8);
    m_client->start();
  }
  return true;
}

bool LookaSearchd::Process(
  const SearchRequest& request, std::string& reply, std::string& extension)
{
  struct timeval parse_start;
  struct timeval segment_start;
  struct timeval search_start;
  int wastetime_parse = 0;
  int wastetime_segment = 0;
  int wastetime_search = 0;
  int wastetime_pack = 0;

  // parse query
  gettimeofday(&parse_start, NULL);
  LookaRequest looka_request;
  looka_request.ParseFilter(request.filter_string());
  looka_request.ParseFilterRange(request.filter_range_string());
  extension = request.dataformat();
  wastetime_parse = WASTE_TIME_US(parse_start);

  // segment query
  gettimeofday(&segment_start, NULL);
  std::vector<SegmentToken> segtokens;
  pthread_mutex_lock(&m_seg_lock);
  m_segmenter->Segment(request.query(), segtokens);
  pthread_mutex_unlock(&m_seg_lock);
  wastetime_segment = WASTE_TIME_US(segment_start);

  // do search
  gettimeofday(&search_start, NULL);
  int total = 0;
  LocalDocID id = 0;
  std::vector<DocAttr*> docs;
  LookaIntersect* inter = new LookaIntersect();
  std::vector<std::string> strtokens;
  for (size_t i=0; strtokens.size()<segtokens.size();
    strtokens.push_back(segtokens[i++].str));
  inter->SetTokens(strtokens, m_inverter);
  while ((id = inter->Seek(id)) != kIllegalLocalDocID) {
    DocAttr*& attr = (*m_summary)[id++];

    // match filter
    if (DropByFilter(attr, looka_request.filter))
      continue;

    // match filter range
    if (DropByFilterRange(attr, looka_request.filter_range))
      continue;

    // match doc
    if (total >= request.offset() && total < request.offset() + request.limit()) {
      docs.push_back(attr);
    }
    total++;
  }
  wastetime_search = WASTE_TIME_US(search_start);

  // pack result
  std::vector<std::pair<std::string, std::string> > extra;
  extra.push_back(
    std::make_pair("total_found", intToString(total)));
  extra.push_back(
    std::make_pair("parse_cost", intToString(wastetime_parse) + "us"));
  extra.push_back(
    std::make_pair("segment_cost", intToString(wastetime_segment) + "us"));
  extra.push_back(
    std::make_pair("search_cost", intToString(wastetime_search) + "us"));

  LookaResultPacker* packer =
    m_result_packer_wrapper->GetResultPacker(request.dataformat());
  reply = packer->PackResult(m_source_cfg, request.query(), strtokens,
    docs, m_attr_names, inter, m_inverter, extra, wastetime_pack);

  delete inter;

  _INFO("[query %s] [filter %s] [filter_range %s] "
    "[total_found %d] [return_num %d] [cost(%d %d %d %d) %dus]",
    request.query().c_str(),
    request.filter_string().c_str(),
    request.filter_range_string().c_str(),
    total,
    static_cast<int>(docs.size()),
    wastetime_parse,
    wastetime_segment,
    wastetime_search,
    wastetime_pack,
    wastetime_parse + wastetime_segment + wastetime_search + wastetime_pack);
  return true;
}

bool LookaSearchd::Process(
  const HttpRequest& request, std::string& reply, std::string& extension)
{
  LookaRequest looka_request;
  if (!looka_request.Parse(request))
    return false;
  SearchRequest search_request;
  if (TransformRequest(looka_request, search_request))
    return false;

  if (m_role == "parent") {
    std::string serialized_str;
    search_request.set_request_id(1);
    if (search_request.SerializeToString(&serialized_str)) {
      const unsigned char* data = (const unsigned char*)&serialized_str[0];
      size_t len = (size_t)serialized_str.length();
      m_client->send(0, data, len, 3);
      return true;
    }
  } else if (m_role == "leaf") {
    return Process(search_request, reply, extension);
  }
  return false;
}

bool LookaSearchd::GetAttrNameIndex(
  const std::string& s, DocAttrType& type, int& index)
{
  if (!m_attr_names)
    return false;
  if (s.empty())
    return false;

  bool found = false;
  std::map<DocAttrType, AttrNames*>::iterator it = m_attr_names->begin();
  for (it; it != m_attr_names->end(); ++it) {
    AttrNames*& attr = it->second;
    for (uint8_t i=0; i<attr->size; i++) {
      if (attr->GetString(i) == s) {
        type = it->first;
        index = static_cast<int>(i);
        found = true;
        break;
      }
    }
  }
  return found;
}

bool LookaSearchd::DropByFilter(
  const DocAttr* attr, const LookaRequest::Filter_t& filter)
{
  bool hit = false;
  int idx;
  DocAttrType type;
  LookaRequest::FilterConstIter_t it;
  for (it=filter.begin(); it!=filter.end(); ++it) {
    const std::string& filter_key = it->first;
    const std::vector<std::string>& filter_values = it->second;
    if (!GetAttrNameIndex(filter_key, type, idx))
      continue;

    std::string s;
    if (type == ATTR_TYPE_UINT) {
      s = StringPrintf("%u", attr->u->data[idx]);
    } else if (type == ATTR_TYPE_FLOAT) {
      s = StringPrintf("%f", attr->f->data[idx]);
    } else if (type == ATTR_TYPE_MULTI) {
      s = StringPrintf("%u", attr->m->data[idx]);
    } else if (type == ATTR_TYPE_STRING) {
      s = attr->s->GetString(idx);
    }

    if (std::find(filter_values.begin(), filter_values.end(), s) ==
      filter_values.end()) {
      hit = true;
      break;
    }
  }
  return hit;
}

bool LookaSearchd::DropByFilterRange(
  const DocAttr* attr, const LookaRequest::FilterRange_t& filter_range)
{
  return false;
}

int LookaSearchd::TcpClientSendCallback(int fd, int server_id, int ret)
{
  _INFO("send fd:%d server_id:%d ret:%d", fd, server_id, ret);
  return 0;
}

int LookaSearchd::TcpClientRecvHandleData(
  const unsigned char* buf, int size, const struct sockaddr_in& addr)
{
  return 0;
}

int LookaSearchd::TcpServerSendCallback(int fd, int ret)
{
  return 0;
}

int LookaSearchd::TcpServerRecvHandleData(
  int fd, const unsigned char* buf, int size, const sockaddr_in& addr)
{
  _INFO("receive a data from fd:%d size:%d", fd, size);
  SearchRequest search_request;
  search_request.ParseFromString(std::string((const char*)buf, size));
  std::string reply;
  std::string extension;
  if (Process(search_request, reply, extension)) {
    // m_server->send(fd, (const unsigned char*)(reply.c_str()), reply.length()+1, 3);
  }
  return 0;
}

int LookaSearchd::TransformRequest(
  const LookaRequest& looka_request, SearchRequest& search_request)
{
  search_request.set_query(looka_request.query);
  search_request.set_index(looka_request.index);
  search_request.set_charset(looka_request.charset);
  search_request.set_dataformat(looka_request.dataformat);
  search_request.set_limit(looka_request.limit);
  search_request.set_offset(looka_request.offset);
  search_request.set_filter_string(looka_request.filter_string);
  search_request.set_filter_range_string(looka_request.filter_range_string);
  return 0;
}
