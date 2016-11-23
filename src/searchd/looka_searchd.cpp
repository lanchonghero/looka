#include <iostream>
#include <string>
#include <pthread.h>
#include <signal.h>
#include <libxml/parser.h>
#include "../looka_file.hpp"
#include "../looka_intersect.hpp"
#include "looka_request.hpp"
#include "looka_searchd.hpp"

LookaSearchd::LookaSearchd(
  LookaConfigSource* source_cfg,
  LookaConfigIndex* index_cfg,
  LookaConfigSearchd* searchd_cfg)
{
  m_source_cfg = source_cfg;
  m_index_cfg = index_cfg;
  m_searchd_cfg = searchd_cfg;

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
  pthread_mutex_destroy(&m_seg_lock);
}

bool LookaSearchd::Init()
{
  _INFO("[reading index & summary ...]");
  LookaIndexReader* reader = new LookaIndexReader();

  reader->ReadIndexFromFile(m_index_cfg->index_file, m_inverter);
  reader->ReadSummaryFromFile(
    m_index_cfg->summary_file_uint,
    m_index_cfg->summary_file_float,
    m_index_cfg->summary_file_multi,
    m_index_cfg->summary_file_string,
    m_summary);
  delete reader;
}

bool LookaSearchd::Process(const HttpRequest& request, std::string& reply, std::string& extension)
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
  LookaRequest req;
  if (!req.Parse(request))
    return false;
  extension = req.dataformat;
  LookaRequest::FilterIter_t fit;
  for (fit = req.filter.begin(); fit != req.filter.end(); ++fit) {
  }
  wastetime_parse = WASTE_TIME_US(parse_start);

  // segment query
  gettimeofday(&segment_start, NULL);
  std::vector<SegmentToken> segtokens;
  pthread_mutex_lock(&m_seg_lock);
  m_segmenter->Segment(req.query, segtokens);
  pthread_mutex_unlock(&m_seg_lock);
  wastetime_segment = WASTE_TIME_US(segment_start);

  // do search
  gettimeofday(&search_start, NULL);
  int total = 0;
  LocalDocID id = 0;
  std::vector<DocAttr*> docs;
  LookaIntersect* inter = new LookaIntersect();
  std::vector<std::string> strtokens;
  for (size_t i=0; strtokens.size()<segtokens.size(); strtokens.push_back(segtokens[i++].str));
  inter->SetTokens(strtokens, m_inverter);
  while ((id = inter->Seek(id)) != kIllegalLocalDocID) {
    if (total >= req.offset && total < req.offset + req.limit) {
      docs.push_back((*m_summary)[id]);
    }
    total++;
    id++;
  }
  wastetime_search = WASTE_TIME_US(search_start);

  // pack result
  std::vector<std::pair<std::string, std::string> > extra;
  extra.push_back(std::make_pair("total_found",  intToString(total)));
  extra.push_back(std::make_pair("parse_cost",   intToString(wastetime_parse) + "us"));
  extra.push_back(std::make_pair("segment_cost", intToString(wastetime_segment) + "us"));
  extra.push_back(std::make_pair("search_cost",  intToString(wastetime_search) + "us"));

  LookaResultPacker* packer = m_result_packer_wrapper->GetResultPacker(req.dataformat);
  reply = packer->PackResult(m_source_cfg, req.query, strtokens, docs, inter, m_inverter, extra, wastetime_pack);

  delete inter;

  _INFO("[query %s] [total_found %d] [return_num %d] [cost(%d %d %d %d) %dus]",
    req.query.c_str(), total, static_cast<int>(docs.size()),
    wastetime_parse,
    wastetime_segment,
    wastetime_search,
    wastetime_pack,
    wastetime_parse + wastetime_segment + wastetime_search + wastetime_pack);

  return true;
}

bool LookaSearchd::IsInArray(const std::string& s, const std::vector<std::string>& array)
{
  const std::vector<std::string>::const_iterator it =
    std::find(array.begin(), array.end(), s);
  return (it != array.end());
}
