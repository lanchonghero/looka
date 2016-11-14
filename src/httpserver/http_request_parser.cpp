#include <stdio.h>
#include "header.hpp"
#include "http_request_parser.hpp"

int HttpRequestParser::ParseHeader(
  HttpRequest& req, const std::string& httpheader)
{
  std::vector<std::string> pieces;
  if (SplitString(httpheader, "\r\n", pieces) == 0)
    return -1;

  // First line in header: GET /xxxx HTTP/1.1
  std::vector<std::string> firstline;
  if (SplitString(pieces[0], " ", firstline) != 3)
    return -1;
  if (sscanf(firstline[2].c_str(), "HTTP/%u.%u",
      &req.http_version_major, &req.http_version_minor) != 2)
    return -1;
  req.method = firstline[0];
  req.uri    = UrlDecode(firstline[1]);

  Header h;
  for (size_t i = 1; i < pieces.size(); i++) {
    if (pieces[i].empty()) continue;
    size_t p = pieces[i].find_first_of(":");
    if (p != std::string::npos) {
      h.name  = Trim(pieces[i].substr(0, p));
      h.value = Trim(pieces[i].substr(p + 1));
      req.headers.push_back(h);
    }
  }
  return 0;
}

int HttpRequestParser::ParseContent(
  HttpRequest& req, const std::string& httpcontent)
{
  req.content = UrlDecode(httpcontent);
  return 0;
}

std::string HttpRequestParser::UrlDecode(const std::string& s)
{
  std::string result;
  int hex = 0;
  for (size_t i = 0; i < s.length(); i++) {
    switch (s[i]) {
    case '+':
      result += ' ';
      break;
    case '%':
      if (isxdigit(s[i + 1]) && isxdigit(s[i + 2])) {
        std::string hexStr = s.substr(i + 1, 2);
        hex = strtol(hexStr.c_str(), 0, 16);
        // 字母和数字[0-9a-zA-Z]、一些特殊符号[$-_.+!*'(),] 、
        // 以及某些保留字[$&+,/:;=?@]可以不经过编码直接用于URL
        if (!((hex >= 48 && hex <= 57) || // 0-9
            (hex >=97 && hex <= 122) || // a-z
            (hex >=65 && hex <= 90) || // A-Z
            //一些特殊符号及保留字[$-_.+!*'(),&/:;=?@]
            hex == 0x21 || hex == 0x24 || hex == 0x26 ||
            hex == 0x27 || hex == 0x28 || hex == 0x29 ||
            hex == 0x2a || hex == 0x2b|| hex == 0x2c ||
            hex == 0x2d || hex == 0x2e || hex == 0x2f ||
            hex == 0x3A || hex == 0x3B|| hex == 0x3D ||
            hex == 0x3f || hex == 0x40 || hex == 0x5f)) {
          result += char(hex);
          i += 2;
        } else {
          result += '%';
        }
      } else {
        result += '%';
      }
      break;
    default:
      result += s[i];
      break;
    }
  }
  return result;
}

size_t HttpRequestParser::SplitString(
  const std::string& str,
  const std::string& split,
  std::vector<std::string>& pieces)
{
  pieces.clear();
  size_t curr_pos = 0;
  size_t last_pos = 0;
  while ((curr_pos = str.find(split, last_pos)) != std::string::npos) {
    std::string piece = str.substr(last_pos, curr_pos - last_pos);
    pieces.push_back(piece);
    last_pos = curr_pos + split.length();
  }
  if (last_pos > 0 && last_pos < str.length()) {
    std::string piece = str.substr(last_pos);
    pieces.push_back(piece);
  }
  return pieces.size();
}

bool HttpRequestParser::IsSpace(const char& c)
{
	if (c == ' ' || c == '\t')
		return true;
	return false;
}

std::string HttpRequestParser::Trim(const std::string& str)
{
	if (str.empty()) return str;
  std::string s = str;
	std::string::iterator it;
	for (it=s.begin(); it!=s.end() && IsSpace(*it++););
	s.erase(s.begin(), --it);
	for (it=s.end(); it!=s.begin() && IsSpace(*--it););
	s.erase(++it, s.end());
  return s;
}
