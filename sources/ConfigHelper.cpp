//##############################################################################
//#              Copyright(c)2022 Turbo Development Design (TDD)               #
//#                         João Rodriguez - VLN37                             #
//#                         Paulo Sergio - psergio-                            #
//#                         Welton Leite - wleite                              #
//##############################################################################

#include "ConfigHelper.hpp"

ConfigHelper::ConfigHelper(void) {
  return;
}

ConfigHelper::ConfigHelper(const ConfigHelper& src) {
  *this = src;
}

ConfigHelper::ConfigHelper(const std::vector<std::string>& tokens) {
  _tokens = tokens;
}

ConfigHelper::~ConfigHelper(void) {
  return;
}

ConfigHelper& ConfigHelper::operator=(const ConfigHelper& rhs) {
  if (this != &rhs) {
    _tokens = rhs._tokens;
  }
  return (*this);
}

int ConfigHelper::get_backlog(void) {
  if (_tokens.size() != 2)
    throw InvalidNumberArgs(_tokens[0]);
  if (String::to_int(_tokens[1]) <= 0 || String::to_int(_tokens[1]) > 4096)
    throw DirectiveInvValue(_tokens[0]);
  return (String::to_int(_tokens[1]));
}

std::pair<in_addr_t, int> ConfigHelper::get_listen(void) {
  in_addr_t ip;
  int port;

  if (String::split(_tokens[1], " ").size() != 1) {
    throw InvalidNumberArgs(_tokens[0]);
  }
  if (_tokens[1].find(":") != std::string::npos) {
    std::vector<std::string> tmp = String::split(_tokens[1], ":");
    if (tmp.size() != 2)
      throw InvFieldValue("host/port", _tokens[1]);
    if (_valid_ip(tmp[0]) && _valid_port(tmp[1])) {
      ip = inet_addr(tmp[0].c_str());
      port = htons(String::to_int(tmp[1]));
    } else if (!_valid_ip(tmp[0])) {
      throw InvFieldValue("host", _tokens[1]);
    } else {
      throw InvFieldValue("port", _tokens[1]);
    }
    return (std::make_pair(ip, port));
  }
  if (_valid_ip(_tokens[1]) && !_valid_port(_tokens[1])) {
    ip = inet_addr(_tokens[1].c_str());
    port = htons(DFL_PORT);
  } else if (!_valid_ip(_tokens[1]) && _valid_port(_tokens[1])) {
    ip = inet_addr(DFL_ADDRESS);
    port = htons(String::to_int(_tokens[1]));
  } else {
    throw InvFieldValue("host/port", _tokens[1]);
  }
  return (std::make_pair(ip, port));
}

std::vector<std::string> ConfigHelper::get_server_name(void) {
  if (_tokens.size() == 1)
    throw InvalidNumberArgs(_tokens[0]);
  std::vector<std::string> tmp(_tokens.begin() + 1, _tokens.end());
  return (tmp);
}

std::string ConfigHelper::get_root(void) {
  if (_tokens.size() == 1)
    throw InvalidNumberArgs(_tokens[0]);
  return (String::trim(std::string(_tokens[1]), "/"));
}

std::vector<std::string> ConfigHelper::get_index(void) {
  if (_tokens.size() == 1)
    throw InvalidNumberArgs(_tokens[0]);
  std::vector<std::string> tmp(_tokens.begin() + 1, _tokens.end());
  return (tmp);
}

std::string ConfigHelper::get_error_page(void) {
  if (_tokens.size() != 3)
    throw InvalidNumberArgs(_tokens[0]);
  if (_tokens[1].find_first_not_of("0123456789") != std::string::npos ||
      String::to_int(_tokens[1]) > 499 || String::to_int(_tokens[1]) < 400) {
    throw InvFieldValue("error_page", _tokens[1]);
  }
  return (String::trim(std::string(_tokens[2]), "/"));
}

int ConfigHelper::get_timeout(void) {
  if (_tokens.size() != 2)
    throw InvalidNumberArgs(_tokens[0]);
  if (String::to_int(_tokens[1]) <= 0 || String::to_int(_tokens[1]) > 4096)
    throw DirectiveInvValue(_tokens[0]);
  return (String::to_int(_tokens[1]));
}

int ConfigHelper::get_client_max_body_size(void) {
  if (_tokens.size() != 2)
    throw InvalidNumberArgs(_tokens[0]);
  if (String::to_int(_tokens[1]) <= 0 || String::to_int(_tokens[1]) > 1000)
    throw DirectiveInvValue(_tokens[0]);
  return (String::to_int(_tokens[1]));
}

std::string ConfigHelper::get_access_log(void) {
  if (_tokens.size() != 2)
    throw InvalidNumberArgs(_tokens[0]);
  return (_tokens[1]);
}

std::string ConfigHelper::get_error_log(void) {
  if (_tokens.size() != 2)
    throw InvalidNumberArgs(_tokens[0]);
  return (_tokens[1]);
}

bool ConfigHelper::get_autoindex(void) {
  if (_tokens.size() != 2)
    throw InvalidNumberArgs(_tokens[0]);
  if (_tokens[1] != "on" && _tokens[1] != "off")
    throw InvFieldValue("autoindex", _tokens[1]);
  return ((_tokens[1] == "on") ? true : false);
}

std::string ConfigHelper::get_cgi(void) {
  if (_tokens.size() != 3)
    throw InvalidNumberArgs(_tokens[0]);
  return (_tokens[2]);
}

std::pair<int, std::string> ConfigHelper::get_redirect(void) {
  if (_tokens.size() != 3)
    throw InvalidNumberArgs(_tokens[0]);
  if (_tokens[1].find_first_not_of("0123456789") != std::string::npos ||
      String::to_int(_tokens[1]) > 499 || String::to_int(_tokens[1]) < 100) {
    throw InvFieldValue("redirect", _tokens[1]);
  }
  return (std::make_pair(String::to_int(_tokens[1]), _tokens[2]));
}

std::vector<std::string> ConfigHelper::get_limit_except(void) {
  if (_tokens.size() == 1)
    throw InvalidNumberArgs(_tokens[0]);
  std::vector<std::string> tmp(_tokens.begin() + 1, _tokens.end());
  for (size_t i = 0; i < tmp.size(); i++) {
    std::transform(tmp[i].begin(), tmp[i].end(), tmp[i].begin(), ::toupper);
  }
  return (tmp);
}

bool ConfigHelper::_valid_ip(const std::string& ip) {
  std::vector<std::string> list = String::split(ip, ".");

  if (list.size() != 4)
    return (false);
  for (size_t i = 0; i < list.size(); i++) {
    if (list[i].find_first_not_of("0123456789") != std::string::npos ||
        String::to_int(list[i]) > 255 || String::to_int(list[i]) < 0) {
      return (false);
    }
  }
  return (true);
}

bool ConfigHelper::_valid_port(const std::string& port) {
  if (port.find_first_not_of("0123456789") != std::string::npos ||
      String::to_int(port) > 65000 || String::to_int(port) < 80) {
    return (false);
  }
  return (true);
}

ConfigHelper::InvalidNumberArgs::InvalidNumberArgs(const std::string& str)
    : LoadException(str) {
  _m = PARSE_ERROR "invalid number of arguments in \"" + str + "\"";
}

const char* ConfigHelper::InvalidNumberArgs::what(void) const throw() {
  return (_m.c_str());
}

ConfigHelper::DirectiveDuplicate::DirectiveDuplicate(const std::string& str)
    : LoadException(str) {
  _m = PARSE_ERROR "directive is duplicate \"" + str + "\"";
}

const char* ConfigHelper::DirectiveDuplicate::what(void) const throw() {
  return (_m.c_str());
}

ConfigHelper::DirectiveInvValue ::DirectiveInvValue(const std::string& str)
    : LoadException(str) {
  _m = PARSE_ERROR "directive invalid value in \"" + str + "\"";
}

const char* ConfigHelper::DirectiveInvValue::what(void) const throw() {
  return (_m.c_str());
}

ConfigHelper::DirectiveUnknown ::DirectiveUnknown(const std::string& str)
    : LoadException(str) {
  _m = PARSE_ERROR "unknown directive \"" + str + "\"";
}

const char* ConfigHelper::DirectiveUnknown::what(void) const throw() {
  return (_m.c_str());
}

ConfigHelper::InvFieldValue::InvFieldValue(const std::string& field,
                                           const std::string& value)
    : LoadException("") {
  _m = PARSE_ERROR "invalid " + field + " in " + "\"" + value + "\"";
}

const char* ConfigHelper::InvFieldValue::what(void) const throw() {
  return (_m.c_str());
}
