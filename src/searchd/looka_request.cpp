#include "looka_request.hpp"
#include "../looka_utils.hpp"

LookaRequest::LookaRequest()
{
  query = "";
  index = "*";
  charset = "utf8";
  dataformat = "xml";
  limit = 4000;
  offset = 0;
}

LookaRequest::~LookaRequest()
{
}

bool LookaRequest::Parse(const HttpRequest& request)
{
  std::string req_str;
  if (request.method == "GET") {
    size_t pos = request.uri.find("/?");
    if (pos != 0)
      return false;
    req_str = request.uri.substr(pos + 2);
  } else if (request.method == "POST") {
    req_str = request.content;
  } else {
    return false;
  }

  std::vector<std::pair<std::string, std::string> > pair_params;
  std::vector<std::string> full_params;
  splitString(req_str, '&', full_params);
  if (full_params.empty())
    return false;
  for (size_t i = 0; i < full_params.size(); i++) {
    std::string argc, argv;
    if (!full_params[i].empty() && !xsplit(full_params[i], argc, argv, '='))
      if (!argc.empty())
        pair_params.push_back(std::make_pair(argc, argv));
  }

  for (size_t i = 0; i < pair_params.size(); i++) {
    std::string key = trim(pair_params[i].first);
    std::string val = trim(pair_params[i].second);
    if (key == "query") {
      query = val;
    } else if (key == "index") {
      index = val;
    } else if (key == "limit") {
      limit = atoi(val.c_str());
    } else if (key == "charset") {
      charset = val;
    } else if (key == "dataformat") {
      dataformat = toLower(val);
    } else if (key == "offset") {
      offset = atoi(val.c_str());
    }
  }
  return true;
}