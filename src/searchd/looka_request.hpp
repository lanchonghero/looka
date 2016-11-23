#ifndef _LOOKA_REQUEST_HPP
#define _LOOKA_REQUEST_HPP
#include <string>
#include <vector>
#include <map>
#include "../httpserver/http_request.hpp"

class LookaRequest
{
public:
  LookaRequest();
  virtual ~LookaRequest();

  bool Parse(const HttpRequest& request);
  bool ParseFilter(const std::string& filter_string);
  bool ParseFilterRange(const std::string& filter_range_string);

public:
  std::string query;
  std::string index;
  std::string charset;
  std::string dataformat;
  int limit;
  int offset;

  typedef std::map<std::string, std::vector<std::string> > Filter_t;
  typedef Filter_t::iterator FilterIter_t;
  Filter_t filter;

  typedef std::map<std::string, std::vector<std::string> > FilterRange_t;
  typedef FilterRange_t::iterator FilterRangeIter_t;
  FilterRange_t filter_range;
};

#endif //_LOOKA_REQUEST_HPP
