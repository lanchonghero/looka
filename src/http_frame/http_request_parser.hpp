#ifndef _HTTP_REQUEST_PARSER_HPP
#define _HTTP_REQUEST_PARSER_HPP
#include "http_request.hpp"

class HttpRequestParser
{
public:
  HttpRequestParser() {}
  virtual ~HttpRequestParser() {}
  
  static int ParseHeader(HttpRequest& req, const std::string& httpheader);
  static int ParseContent(HttpRequest& req, const std::string& httpcontent);
  static std::string UrlDecode(const std::string& s);

private:
  static std::string Trim(const std::string& str);
  static bool IsSpace(const char& c);
  static size_t SplitString(
    const std::string& str,
    const std::string& split,
    std::vector<std::string>& pieces);
};

#endif //_HTTP_REQUEST_PARSER_HPP
