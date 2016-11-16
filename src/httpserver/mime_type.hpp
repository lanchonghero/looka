#ifndef _MIME_TYPE_HPP
#define _MIME_TYPE_HPP
#include <string>
#include <map>

class MimeType
{
public:
  MimeType() {}
  virtual ~MimeType() {}

  static std::string ExtensionToType(const std::string& extension) {
    std::map<std::string, std::string>::const_iterator it;
    it = extension_mimetype_map.find(extension);
    if (it != extension_mimetype_map.end())
      return it->second;
    return "text/plain";
  }
  
private:
  static std::map<std::string, std::string> InitExtensionMimeTypeMap() {
    std::map<std::string, std::string> ext_mime_map;
    ext_mime_map["gif"]   = "image/gif";
    ext_mime_map["htm"]   = "text/html";
    ext_mime_map["html"]  = "text/html";
    ext_mime_map["jpg"]   = "image/jpeg";
    ext_mime_map["png"]   = "image/png";
    ext_mime_map["xml"]   = "text/xml";
    ext_mime_map["json"]  = "application/json;charset=utf8";
    return ext_mime_map;
  }

private:
  static const std::map<std::string, std::string> extension_mimetype_map;
};

const std::map<std::string, std::string> MimeType::extension_mimetype_map =
MimeType::InitExtensionMimeTypeMap();

#endif //_MIME_TYPE_HPP
