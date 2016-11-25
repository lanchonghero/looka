#ifndef _LOOKA_REQUEST_HPP
#define _LOOKA_REQUEST_HPP
#include <string>
#include <vector>
#include <map>
#include "../http_frame/http_request.hpp"

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
  std::string filter_string;
  std::string filter_range_string;
  int limit;
  int offset;

  typedef std::map<std::string, std::vector<std::string> > Filter_t;
  typedef Filter_t::const_iterator FilterConstIter_t;
  typedef Filter_t::iterator FilterIter_t;
  Filter_t filter;

  typedef std::map<std::string, std::vector<std::string> > FilterRange_t;
  typedef FilterRange_t::const_iterator FilterRangeConstIter_t;
  typedef FilterRange_t::iterator FilterRangeIter_t;
  FilterRange_t filter_range;
};

#endif //_LOOKA_REQUEST_HPP
