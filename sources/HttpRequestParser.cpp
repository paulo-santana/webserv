#include "HttpRequestParser.hpp"
#include "HttpRequest.hpp"
#include "WebServ.hpp"
#include <cctype>
#include <cstddef>
#include <sys/socket.h>
#include <exception>

inline bool is_space(char c) {
  return (c == ' ');
}

inline bool is_lalpha(char c) {
  return (c >= 'a' && c <= 'z');
}

inline bool is_ualpha(char c) {
  return (c >= 'A' && c <= 'Z');
}

inline bool is_alpha(char c) {
  return (is_ualpha(c) || is_lalpha(c));
}

inline bool is_ctl(char c) {
  return ((c >= 0 && c < 32) || c == 127);
}

inline bool is_character(char c) {
  return (std::isdigit(c) || is_alpha(c));
}

inline bool is_crlf(char c) {
  return (c == '\r' && (c + 1) == '\n');
}

bool is_separator(char c) {
  switch (c) {
    case '(':
    case ')':
    case '<':
    case '>':
    case '@':
    case ',':
    case ';':
    case ':':
    case '"':
    case '/':
    case '[':
    case ']':
    case '?':
    case '=':
    case '{':
    case '}':
    case ' ':
    case '\\':
    case '\t':
      return true;
    default:
      return false;
  }
}

// LWS = linear white space
inline bool is_lws(char *c) {
  int offset = 0;
  if (is_crlf(*c))
    offset = 1;
  char target = *(c + offset);
  return (target == ' ' || target == '\t');
}

inline bool is_ascii(char c) {
  return (c >= 0);
}

inline bool is_token(char c) {
  return (is_ascii(c) && !is_ctl(c) && !is_separator(c));
}

std::string HttpRequestParser::supported_version = "HTTP/1.1";

HttpRequestParser::HttpRequestParser(int fd, size_t buff_max):
  finished(false),
  fd(fd),
  buffer(new char[buff_max]),
  bytes_read(),
  buff_max(buff_max),
  current_state(S_INIT),
  supported_version_index(0) {
    _request = new Request();
  }

HttpRequestParser::HttpRequestParser(const HttpRequestParser &other):
  finished(false),
  fd(other.fd),
  buffer(new char[other.buff_max]),
  bytes_read(other.bytes_read),
  buff_max(other.buff_max),
  current_state(other.current_state),
  supported_version_index(other.supported_version_index) { }

HttpRequestParser& HttpRequestParser::operator=(const HttpRequestParser &other) {
  delete[] buffer;
  buffer = new char[other.buff_max];
  buff_max = other.buff_max;
  bytes_read = other.bytes_read;
  finished = other.finished;
  return *this;
}

HttpRequestParser::~HttpRequestParser() {
  delete[] buffer;
  delete _request;
}

/*
 * the request header starts with
 * METHOD /uri.php HTTP/1.1\r\n
 * */

void HttpRequestParser::tokenize_partial_request(char *buff) {
  size_t i = 0;

  while (i < bytes_read) {
    char c = buff[i++];
    WebServ::log.debug() << "current byte: " << c << std::endl;
    // WebServ::log.info() << "current request: " << *_request << std::endl;

    switch (current_state) {
      case S_INIT:
      case S_METHOD_START:
        if (!is_ualpha(c)) {
          throw InvalidHttpRequestException();
        }
        _request->method.push_back(c);
        current_state = S_METHOD;
        break;

      case S_METHOD:
        if (is_space(c)) {
          current_state = S_URI_START;
        } else if (!is_ualpha(c)) {
          throw InvalidHttpRequestException();
        } else {
          _request->method.push_back(c);
        }
        break;

      case S_URI_START:
        if (is_ctl(c)) {
          throw InvalidHttpRequestException();
        }
        _request->path.push_back(c);
        current_state = S_URI;
        break;

      case S_URI:
        if(is_space(c)) {
          current_state = S_HTTP_VERSION;
        } else if (is_ctl(c)) {
          throw InvalidHttpRequestException();
        } else {
          _request->path.push_back(c);
        }
        break;

      case S_HTTP_VERSION:
        {
          size_t idx = supported_version_index;
          if (idx >= supported_version.size()) {
            _request->http_version = supported_version;
            current_state = S_REQUEST_LINE_CRLF;
          } else if (c != supported_version[idx]) {
            throw InvalidHttpRequestException();
          } else {
            supported_version_index++;
          }
        }
        break;

      case S_REQUEST_LINE_CRLF:
        if (c == '\r') {
          current_state = S_REQUEST_LINE_LF;
        } else if (c == '\n') {
          current_state = S_HEADER_LINE_START;
        } else 
            throw InvalidHttpRequestException();
        break;

      case S_REQUEST_LINE_LF:
        if (c == '\n') {
          current_state = S_HEADER_LINE_START;
        } else 
            throw InvalidHttpRequestException();
        break;

      case S_HEADER_LINE_START:
        if (c == '\r') {
          current_state = S_HEADERS_END_LF;
        } else if (c == '\n') {
          current_state = S_BODY_START;
        } else if (!is_character(c)) {
            throw InvalidHttpRequestException();
        } else {
          _header_key.clear();
          WebServ::log.debug()
            << "header key capacity after clear: "
            << _header_key.capacity() << std::endl;
          _header_key.push_back(c);
          current_state = S_HEADER_LINE_KEY;
        }
        break;

      case S_HEADER_LINE_KEY:
        if (c == ':') {
          _request->headers[_header_key];
          current_state = S_HEADER_LINE_SPACE;
        } else if (is_ctl(c) || is_separator(c)) {
          throw InvalidHttpRequestException();
        } else {
          _header_key.push_back(c);
        }
        break;

      case S_HEADER_LINE_SPACE:
        if (c != ' ') {
          throw InvalidHttpRequestException();
        } else {
          _header_value.clear();
          current_state = S_HEADER_LINE_VALUE;
        }
        break;

      case S_HEADER_LINE_VALUE:
        if (c == '\r') { // header value finished
          current_state = S_HEADER_LINE_LF;
        } else if (c == '\n') {
          current_state = S_HEADER_LINE_START;
        } else if (is_ctl(c)) {
          throw InvalidHttpRequestException();
        } else {
          _header_value.push_back(c);
          current_state = S_HEADER_LINE_VALUE;
        }
        break;

      case S_HEADER_LINE_LF:
        if (c != '\n')
          throw InvalidHttpRequestException();

        _request->headers[_header_key] = _header_value;
        current_state = S_HEADER_LINE_START;
        break;

      case S_HEADERS_END_LF:
        if (c != '\n')
          throw InvalidHttpRequestException();
        current_state = S_BODY_START;
        break;

      case S_BODY_START:
        WebServ::log.debug() << "Initializing body parsing" << std::endl;
        WebServ::log.debug() << "Current request: " << *_request << std::endl;
        current_state = S_BODY;
        break;
      case S_BODY:
        break;
      default:
        throw std::exception();
    }
    if (i == bytes_read) {
      return;
    }
  }
  finished = true;
}

void HttpRequestParser::parse() {
  if (finished)
    throw std::exception();

  bytes_read = recv(fd, buffer, buff_max, MSG_WAITALL);

  if (bytes_read == (size_t)-1) {
    perror("recv");
    throw std::exception(); // TODO: implement proper error handling
  } else if (bytes_read == 0) {
    WebServ::log.error() << "recv returned 0" << std::endl;
    throw std::exception(); // TODO: implement proper error handling
  }

  try {
    tokenize_partial_request(buffer);
  } catch(InvalidHttpRequestException& ) {
    WebServ::log.warning() << "Invalid http request" << std::endl;
  } catch(std::exception& e) {
    WebServ::log.error() << "Unexpected exception: " << e.what() << std::endl;
  }
}

Request &HttpRequestParser::get_request() {
  _request->method = "GET";
  _request->path = "/index.html";
  _request->http_version = "HTTP/1.1";
  _request->host = "localhost:3492";
  _request->finished = true;
  _request->valid = true;
  return *_request;
}

HttpRequestParser::Token::Token(const char *value, size_t size, TokenType type)
  : value(value), size(size), type(type) { }
