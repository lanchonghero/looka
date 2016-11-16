#ifndef _LOOKA_REQUEST_HPP
#define _LOOKA_REQUEST_HPP
#include <string>
#include <vector>
#include "../httpserver/http_request.hpp"

class LookaRequest
{
public:
  LookaRequest();
  virtual ~LookaRequest();

  bool Parse(const HttpRequest& request);

public:
  std::string query;
  std::string index;
  std::string charset;
  std::string dataformat;
  int limit;
  int offset;
};

#endif //_LOOKA_REQUEST_HPP
