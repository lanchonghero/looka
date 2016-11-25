#ifndef _HTTP_REPLY_HPP
#define _HTTP_REPLY_HPP
#include <string>
#include <vector>
#include <map>
#include "header.hpp"


class HttpReply {
public:
  HttpReply();
  virtual ~HttpReply();

  enum StatusType {
    STATUS_OK = 200,
    STATUS_CREATED = 201,
    STATUS_ACCEPTED = 202,
    STATUS_NO_CONTENT = 204,
    STATUS_MULTIPLE_CHOICES = 300,
    STATUS_MOVED_PERMANENTLY = 301,
    STATUS_MOVED_TEMPORARILY = 302,
    STATUS_NOT_MODIFIED = 304,
    STATUS_BAD_REQUEST = 400,
    STATUS_UNAUTHORIZED = 401,
    STATUS_FORBIDDEN = 403,
    STATUS_NOT_FOUND = 404,
    STATUS_INTERNAL_SERVER_ERROR = 500,
    STATUS_NOT_IMPLEMENTED = 501,
    STATUS_BAD_GATEWAY = 502,
    STATUS_SERVICE_UNAVAILABLE = 503
  };
public:
  std::string ToString();
  int AddHeader(const std::string& name, const std::string& value);
  
public:
  static std::string StatusToString(StatusType st);
  static std::string StatusToHtmlString(StatusType st);
  static HttpReply DirectReply(StatusType st);

public:
  StatusType status;
  std::vector<Header> headers;
  std::string content;

private:
  static std::map<StatusType, std::string> InitStatusString();
  static std::map<StatusType, std::string> InitStatusHtmlString();

private:
  static const std::map<StatusType, std::string> static_status_string;
  static const std::map<StatusType, std::string> static_status_html_string;

  static const std::string static_name_value_separator;
  static const std::string static_crlf;
};

#endif //_HTTP_REPLY_HPP
