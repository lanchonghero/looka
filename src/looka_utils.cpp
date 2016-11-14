#include <sstream>
#include <iconv.h>
#include <string.h>
#include "looka_utils.hpp"

std::string& ltrimc(std::string &s, const char c) {
	std::string::iterator it;
	for (it=s.begin(); it!=s.end() && (c==*it++);); s.erase(s.begin(), --it);
	return s;
}

std::string& rtrimc(std::string &s, const char c) {
	std::string::iterator it;
	for (it=s.end(); it!=s.begin() && (c==*--it);); s.erase(++it, s.end());
	return s;
}

std::string& trimc(std::string &s, const char c) {
	return ltrimc(rtrimc(s, c), c);
}

std::string& ltrim(std::string &s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
	return s;
}

std::string& rtrim(std::string &s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
	return s;
}

std::string& trim(std::string &s) {
	return ltrim(rtrim(s));
}

std::string& replace(std::string &s, const std::string &find_s, const std::string &replace_s) {
	size_t pos = 0;
	while (pos < s.length() && (pos=s.find_first_of(find_s, pos)) != std::string::npos) {
		s = s.replace(pos, find_s.length(), replace_s);
		pos += replace_s.length();
	}
	return s;
}

void splitString(const std::string& s, char ch, std::vector<std::string>& vs)
{
	size_t a = 0;
	size_t b;
	for (b = 0; s[b] != '\0'; ++ b) {
		if (s[b] == ch) {
			std::string ss = s.substr(a, b - a);
			vs.push_back(trim(ss));
			a = b + 1;
		}
	}
	if (a < b) {
		std::string ss = s.substr(a, b - a);
		vs.push_back(trim(ss));
	}
}

bool strfind(const std::string &s, const std::string &f)
{
	return !(s.find(f)==std::string::npos);
}

int xsplit(const std::string& in, std::string& out1, std::string& out2, const char sep)
{
  size_t pos = in.find(sep);
  if (pos == std::string::npos) {
    return -1;
  }
  out1 = in.substr(0, pos);
  out2 = in.substr(pos + 1);
  return 0;
}

std::string& toLower(std::string& s)
{
  transform(s.begin(), s.end(), s.begin(), tolower);
  return s;
}

std::string& toUpper(std::string& s)
{
  transform(s.begin(), s.end(), s.begin(), toupper);
  return s;
}

std::string intToString(int i)
{
  std::string s;
  std::stringstream ss;
  ss << i; 
  s = ss.str();
  return s;
}

int code_convert(
  const std::string& from_charset,
  const std::string& to_charset,
  const std::string& in,
  std::string& out)
{
  std::string f(from_charset);
  std::string t(to_charset);
  toLower(f);
  toLower(t);
  if (!(f == "utf8" || f == "utf-8") || t != "gbk")
    return -1;
  if (in.empty())
    return -1;

  iconv_t cd = iconv_open(t.c_str(), f.c_str());
  if (cd == (iconv_t)-1) {
    fprintf(stdout, "iconv_open false: %s ==> %s\n", f.c_str(), t.c_str());
    return -1;
  }

  int ret = 0;
  size_t in_len  = in.length();
  size_t out_len = in.length() * 2;
  char buffer[out_len];

  char* ref_out  = buffer;
  char* ref_in   = const_cast<char*>(in.c_str());
  char **ptr_in  = &ref_in;
  char **ptr_out = &ref_out;
  if (iconv(cd, ptr_in, &in_len, ptr_out, &out_len) == (size_t)-1)
    ret = -1;
  else
    out = std::string(buffer, out_len);

  iconv_close(cd);
  return 0;
}

int utf8_to_gbk(const std::string& in, std::string& out)
{
  return code_convert("utf-8", "gbk", in, out);
}

int gbk_to_utf8(const std::string& in, std::string& out)
{
  return code_convert("gbk", "utf-8", in, out);
}
