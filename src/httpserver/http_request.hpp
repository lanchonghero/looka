#ifndef _HTTP_REQUEST_HPP
#define _HTTP_REQUEST_HPP
#include <string>
#include <vector>
#include "header.hpp"

class HttpRequest
{
public:
  HttpRequest():
    method(""), uri(""),
    http_version_major(0), http_version_minor(0), content("") {
  }
  virtual ~HttpRequest() {}

  std::string GetHeaderValue(const std::string& name) {
    for (size_t i = 0; i < headers.size(); i++) {
      if (headers[i].name == name)
        return headers[i].value;
    }
    return "";
  }

public:
  std::string method;
  std::string uri;
  unsigned int http_version_major;
  unsigned int http_version_minor;
  std::vector<Header> headers;
  std::string content;
};

#endif //_HTTP_REQUEST_HPP
