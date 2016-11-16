#include <iconv.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <sstream>
#include "looka_string_utils.hpp"

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

std::string& ltrimc(std::string &s, const std::string &cs)
{
  std::string::iterator it;
  for (it=s.begin(); it!=s.end() && (cs.find(*it++)!=std::string::npos);); s.erase(s.begin(), --it); 
  return s;
}

std::string& rtrimc(std::string &s, const std::string &cs)
{
  std::string::iterator it;
  for (it=s.end(); it!=s.begin() && (cs.find(*--it)!=std::string::npos);); s.erase(++it, s.end());
  return s;
}

std::string& trimc(std::string&s, const std::string &cs)
{
  return ltrimc(rtrimc(s, cs), cs);
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
  int pos = -1;
  while ((pos = s.find(find_s)) != -1) {
    s = s.replace(pos, find_s.length(), replace_s);
  }
  return s;
}

std::string& quotstr(std::string &s, const char c) {
  s.insert(s.begin(), c);
  s.insert(s.end(), c);
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

static void StringPrintfV(std::string* dst, const char* format, va_list ap) {
  errno = 0;
  char stack_buf[1024];

  va_list ap_copy;
  va_copy(ap_copy, ap);
  int result = vsnprintf(stack_buf, sizeof(stack_buf), format, ap_copy);
  va_end(ap_copy);

  if (result >= 0 && result < static_cast<int>(sizeof(stack_buf))) {
    // It fit.
    dst->assign(stack_buf, result);
    return;
  }

  // Repeatedly increase buffer size until it fits.
  int mem_length = sizeof(stack_buf);
  while (true) {
    if (result < 0) {
      // On Windows, vsnprintfT always returns the number of characters in a
      // fully-formatted string, so if we reach this point, something else is
      // wrong and no amount of buffer-doubling is going to fix it.
      if (errno != 0 && errno != EOVERFLOW) {
        // If an error other than overflow occurred, it's never going to work.
        fprintf(stderr, "Unable to printf the requested string due to error.\n");
        return;
      }
      // Try doubling the buffer size.
      mem_length *= 2;
    } else {
      // We need exactly "result + 1" characters.
      mem_length = result + 1;
    }

    if (mem_length > 32 * 1024 * 1024) {
      // That should be plenty, don't try anything larger.  This protects
      // against huge allocations when using vsnprintfT implementations that
      // return -1 for reasons other than overflow without setting errno.
      fprintf(stderr, "Unable to printf the requested string due to size.\n");
      return;
    }

    std::vector<char> mem_buf(mem_length);

    // NOTE: You can only use a va_list once.  Since we're in a while loop, we
    // need to make a new copy each time so we don't use up the original.
    va_list ap_copy;
    va_copy(ap_copy, ap);
    result = vsnprintf(&mem_buf[0], mem_length, format, ap_copy);
    va_end(ap_copy);

    if ((result >= 0) && (result < mem_length)) {
      // It fit.
      dst->assign(&mem_buf[0], result);
      return;
    }
  }
}

const std::string StringPrintf(const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  std::string result;
  StringPrintfV(&result, format, ap);
  va_end(ap);
  return result;
}


std::string UrlEncode(const std::string& s)
{
  std::string dst;
  std::string src = s;
  char hex[] = "0123456789ABCDEF";
  for (size_t i = 0; i < src.size(); i++) {
    unsigned char cc = src[i];
    if (isascii(cc)) {
      if (cc == ' ') {
        dst += "%20";
      } else {
        dst += cc;
      }
    } else {
      unsigned char c = static_cast<unsigned char>(src[i]);
      dst += '%';
      dst += hex[c / 16];
      dst += hex[c % 16];
    }
  }
  return dst;
};

std::string UrlDecode(const std::string& s)
{
  std::string result;
  int hex = 0;
  for (size_t i = 0; i < s.length(); i++) {
    switch (s[i])
    {
    case '+':
      result += ' ';
      break;
    case '%':
      if (isxdigit(s[i + 1]) && isxdigit(s[i + 2])) {
        std::string hexStr = s.substr(i + 1, 2);
        hex = strtol(hexStr.c_str(), 0, 16);
        //字母和数字[0-9a-zA-Z]、一些特殊符号[$-_.+!*'(),] 、以及某些保留字[$&+,/:;=?@]
        //可以不经过编码直接用于URL
        if (!((hex >= 48 && hex <= 57) || //0-9
            (hex >=97 && hex <= 122) ||   //a-z
            (hex >=65 && hex <= 90) ||    //A-Z
            //一些特殊符号及保留字[$-_.+!*'(),]  [$&+,/:;=?@]
            hex == 0x21 || hex == 0x24 || hex == 0x26 || hex == 0x27 || hex == 0x28 || hex == 0x29
            || hex == 0x2a || hex == 0x2b|| hex == 0x2c || hex == 0x2d || hex == 0x2e || hex == 0x2f
            || hex == 0x3A || hex == 0x3B|| hex == 0x3D || hex == 0x3f || hex == 0x40 || hex == 0x5f
            )) {
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
