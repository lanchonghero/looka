#include <sstream>
#include "http_reply.hpp"

const std::map<HttpReply::StatusType, std::string>
HttpReply::static_status_string = InitStatusString();

const std::map<HttpReply::StatusType, std::string>
HttpReply::static_status_html_string = InitStatusHtmlString();

const std::string HttpReply::static_name_value_separator = ": ";
const std::string HttpReply::static_crlf = "\r\n";

HttpReply::HttpReply(): status(STATUS_OK), content("")
{
}

HttpReply::~HttpReply()
{
}

std::string HttpReply::ToString()
{
  std::string reply_string;
  reply_string.append(HttpReply::StatusToString(status));
  for (size_t i = 0; i < headers.size(); i++) {
    Header& h = headers[i];
    reply_string.append(h.name);
    reply_string.append(HttpReply::static_name_value_separator);
    reply_string.append(h.value);
    reply_string.append(HttpReply::static_crlf);
  }
  reply_string.append(HttpReply::static_crlf);
  reply_string.append(content);
  return reply_string;
}

int HttpReply::AddHeader(const std::string& name, const std::string& value)
{
  if (name.empty())
    return -1;
  Header h;
  h.name = name;
  h.value = value;
  headers.push_back(h);
}

std::string HttpReply::StatusToString(StatusType st)
{
  std::map<StatusType, std::string>::const_iterator it =
    static_status_string.find(st);
  if (it != static_status_string.end())
    return it->second;
  return "HTTP/1.1 500 Internal Server Error\r\n";
}

std::string HttpReply::StatusToHtmlString(StatusType st)
{
  std::map<StatusType, std::string>::const_iterator it =
    static_status_html_string.find(st);
  if (it != static_status_html_string.end())
    return it->second;
  return "";
}

HttpReply HttpReply::DirectReply(StatusType st)
{
  HttpReply reply;
  reply.status = st;
  reply.content = HttpReply::StatusToHtmlString(st);
  std::stringstream ss_content_length;
  ss_content_length << reply.content.length();
  reply.AddHeader("Content-Length", ss_content_length.str());
  reply.AddHeader("Content-Type", "text/html");
  return reply;
}

std::map<HttpReply::StatusType, std::string> HttpReply::InitStatusString()
{
  std::map<StatusType, std::string> status_string;
  status_string[STATUS_OK] =
    "HTTP/1.1 200 OK\r\n";
  status_string[STATUS_CREATED] =
    "HTTP/1.1 201 Created\r\n";
  status_string[STATUS_ACCEPTED] =
    "HTTP/1.1 202 Accepted\r\n";
  status_string[STATUS_NO_CONTENT] =
    "HTTP/1.1 204 No Content\r\n";
  status_string[STATUS_MULTIPLE_CHOICES] =
    "HTTP/1.1 300 Multiple Choices\r\n";
  status_string[STATUS_MOVED_PERMANENTLY] =
    "HTTP/1.1 301 Moved Permanently\r\n";
  status_string[STATUS_MOVED_TEMPORARILY] =
    "HTTP/1.1 302 Moved Temporarily\r\n";
  status_string[STATUS_NOT_MODIFIED] =
    "HTTP/1.1 304 Not Modified\r\n";
  status_string[STATUS_BAD_REQUEST] =
    "HTTP/1.1 400 Bad Request\r\n";
  status_string[STATUS_UNAUTHORIZED] =
    "HTTP/1.1 401 Unauthorized\r\n";
  status_string[STATUS_FORBIDDEN] =
    "HTTP/1.1 403 Forbidden\r\n";
  status_string[STATUS_NOT_FOUND] =
    "HTTP/1.1 404 Not Found\r\n";
  status_string[STATUS_INTERNAL_SERVER_ERROR] =
    "HTTP/1.1 500 Internal Server Error\r\n";
  status_string[STATUS_NOT_IMPLEMENTED] =
    "HTTP/1.1 501 Not Implemented\r\n";
  status_string[STATUS_BAD_GATEWAY] =
    "HTTP/1.1 502 Bad Gateway\r\n";
  status_string[STATUS_SERVICE_UNAVAILABLE] =
    "HTTP/1.1 503 Service Unavailable\r\n";
  return status_string;
}

std::map<HttpReply::StatusType, std::string> HttpReply::InitStatusHtmlString()
{
  std::map<StatusType, std::string> status_html_string;
  status_html_string[STATUS_OK] = "";
  status_html_string[STATUS_CREATED] =
    "<html>"
    "<head><title>Created</title></head>"
    "<body><h1>201 Created</h1></body>"
    "</html>";
  status_html_string[STATUS_ACCEPTED] =
    "<html>"
    "<head><title>Accepted</title></head>"
    "<body><h1>202 Accepted</h1></body>"
    "</html>";
  status_html_string[STATUS_NO_CONTENT] =
    "<html>"
    "<head><title>No Content</title></head>"
    "<body><h1>204 Content</h1></body>"
    "</html>";
  status_html_string[STATUS_MULTIPLE_CHOICES] =
    "<html>"
    "<head><title>Multiple Choices</title></head>"
    "<body><h1>300 Multiple Choices</h1></body>"
    "</html>";
  status_html_string[STATUS_MOVED_PERMANENTLY] =
    "<html>"
    "<head><title>Moved Permanently</title></head>"
    "<body><h1>301 Moved Permanently</h1></body>"
    "</html>";
  status_html_string[STATUS_MOVED_TEMPORARILY] =
    "<html>"
    "<head><title>Moved Temporarily</title></head>"
    "<body><h1>302 Moved Temporarily</h1></body>"
    "</html>";
  status_html_string[STATUS_NOT_MODIFIED] =
    "<html>"
    "<head><title>Not Modified</title></head>"
    "<body><h1>304 Not Modified</h1></body>"
    "</html>";
  status_html_string[STATUS_BAD_REQUEST] =
    "<html>"
    "<head><title>Bad Request</title></head>"
    "<body><h1>400 Bad Request</h1></body>"
    "</html>";
  status_html_string[STATUS_UNAUTHORIZED] =
    "<html>"
    "<head><title>Unauthorized</title></head>"
    "<body><h1>401 Unauthorized</h1></body>"
    "</html>";
  status_html_string[STATUS_FORBIDDEN] =
    "<html>"
    "<head><title>Forbidden</title></head>"
    "<body><h1>403 Forbidden</h1></body>"
    "</html>";
  status_html_string[STATUS_NOT_FOUND] =
    "<html>"
    "<head><title>Not Found</title></head>"
    "<body><h1>404 Not Found</h1></body>"
    "</html>";
  status_html_string[STATUS_INTERNAL_SERVER_ERROR] =
    "<html>"
    "<head><title>Internal Server Error</title></head>"
    "<body><h1>500 Internal Server Error</h1></body>"
    "</html>";
  status_html_string[STATUS_NOT_IMPLEMENTED] =
    "<html>"
    "<head><title>Not Implemented</title></head>"
    "<body><h1>501 Not Implemented</h1></body>"
    "</html>";
  status_html_string[STATUS_BAD_GATEWAY] =
    "<html>"
    "<head><title>Bad Gateway</title></head>"
    "<body><h1>502 Bad Gateway</h1></body>"
    "</html>";
  status_html_string[STATUS_SERVICE_UNAVAILABLE] =
    "<html>"
    "<head><title>Service Unavailable</title></head>"
    "<body><h1>503 Service Unavailable</h1></body>"
    "</html>";
  return status_html_string;
}
