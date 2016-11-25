#ifndef _LOOKA_SEARCHD_HPP
#define _LOOKA_SEARCHD_HPP

#include "../looka_log.hpp"
#include "../looka_types.hpp"
#include "../looka_segmenter.hpp"
#include "../looka_config_index.hpp"
#include "../looka_config_searchd.hpp"
#include "../looka_config_source.hpp"
#include "../looka_inverter.hpp"
#include "../http_frame/server_handler.hpp"
#include "../tcp_frame/tcp_client.hpp"
#include "../tcp_frame/tcp_server.hpp"
#include "SearchRequest.pb.h"
#include "looka_result_packer.hpp"
#include "looka_request.hpp"

class LookaSearchd: public ServerHandler
{
public:
  LookaSearchd(
    LookaConfigSource* source_cfg,
    LookaConfigIndex* index_cfg,
    LookaConfigSearchd* searchd_cfg);
  virtual ~LookaSearchd();

  bool Init();
  virtual bool Process(
    const HttpRequest& request, std::string& reply, std::string& extension);

  virtual bool Process(
    const SearchRequest& request, std::string& reply, std::string& extension);

private:
  bool DropByFilter(const DocAttr* attr, const LookaRequest::Filter_t& filter);
  bool DropByFilterRange(
    const DocAttr* attr, const LookaRequest::FilterRange_t& filter_range);
  bool GetAttrNameIndex(const std::string& s, DocAttrType& type, int& index);

  int TransformRequest(const LookaRequest& looka_request, SearchRequest& search_request);

  int TcpClientSendCallback(int fd, int server_id, int ret);
  int TcpClientRecvHandleData(const unsigned char* buf, int size, const struct sockaddr_in& addr);

  int TcpServerSendCallback(int fd, int ret);
  int TcpServerRecvHandleData(int fd, const unsigned char* buf, int size, const sockaddr_in& addr);

public:
  LookaConfigSource*  m_source_cfg;
  LookaConfigIndex*   m_index_cfg;
  LookaConfigSearchd* m_searchd_cfg;

  LookaSegmenter*     m_segmenter;
  LookaInverter<Token, DocInvert*>* m_inverter;
  std::vector<DocAttr*>* m_summary;
  LookaResultPackerWrapper* m_result_packer_wrapper;
  std::map<DocAttrType, AttrNames*>* m_attr_names;

  tcp_client<LookaSearchd, LookaSearchd>* m_client;
  tcp_server<LookaSearchd, LookaSearchd>* m_server;
  std::string m_role;

  pthread_mutex_t m_seg_lock;
};

#endif //_LOOKA_SEARCHD_HPP
