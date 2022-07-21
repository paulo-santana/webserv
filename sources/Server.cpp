//##############################################################################
//#              Copyright(c)2022 Turbo Development Design (TDD)               #
//#                         João Rodriguez - VLN37                             #
//#                         Paulo Sergio - psergio-                            #
//#                         Welton Leite - wleite                              #
//##############################################################################

#include "Server.hpp"

Server::Server(void) {
  ip = inet_addr(DFL_ADDRESS);
  port = htons(DFL_PORT);
  server_name.push_back(DFL_SERVER_NAME1);
  server_name.push_back(DFL_SERVER_NAME2);
  root = DFL_SERVER_ROOT;
  index.push_back(DFL_SERVER_INDEX_PAGE1);
  index.push_back(DFL_SERVER_INDEX_PAGE2);
  error_page[404] = DFL_404_PAGE;
  error_page[405] = DFL_405_PAGE;
  timeout = DFL_TIMEOUT;
  client_max_body_size = DFL_CLI_MAX_BODY_SIZE;
  log = std::map<std::string, std::string>();
  cgi = std::map<std::string, std::string>();
  redirect = std::make_pair(0, "");
  location = std::map<std::string, server_location>();
  autoindex = DFL_AUTO_INDEX;
  sockfd = DFL_SOCK_FD;
}

Server::Server(const Server& src) {
  *this = src;
}

Server::~Server(void) {
  if (sockfd > 0)
    close(sockfd);
}

Server& Server::operator=(const Server& rhs) {
  if (this != &rhs) {
    ip = rhs.ip;
    port = rhs.port;
    server_name = rhs.server_name;
    root = rhs.root;
    index = rhs.index;
    error_page = rhs.error_page;
    timeout = rhs.timeout;
    client_max_body_size = rhs.client_max_body_size;
    log = rhs.log;
    cgi = rhs.cgi;
    redirect = rhs.redirect;
    location = rhs.location;
    autoindex = rhs.autoindex;
    sockfd = rhs.sockfd;
  }
  return (*this);
}

void Server::_socket(void) {
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1) {
    perror("socket");
    exit(errno);
  }
  fcntl(sockfd, F_SETFL, O_NONBLOCK);
}

void Server::_bind(void) {
  struct sockaddr_in sockaddress;

  sockaddress.sin_family = AF_INET;
  sockaddress.sin_port = port;
  sockaddress.sin_addr.s_addr = ip;

  int yes = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes)) {
    perror("setsockopt");
    exit(42);
  }
  if (bind(sockfd, (const sockaddr*)&sockaddress, sizeof(sockaddr_in))) {
    perror("bind");
    close(sockfd);
    exit(errno);
  }
}

void Server::_listen(int backlog) {
  if (listen(sockfd, backlog)) {
    perror("listen");
    exit(errno);
  }
}

int Server::_connect(int backlog) {
  _socket();
  _bind();
  _listen(backlog);
  return 0;
}

void Server::print(void) {
  static size_t n = 1;

  std::cout << "====VHOST " << n << "====\n";

  in_addr t;
  t.s_addr = ip;
  std::cout << "ip: =>" << inet_ntoa(t) << "<=\n";

  std::cout << "port: =>" << ntohs(port) << "<=\n";

  for (size_t i = 0; i < server_name.size(); i++) {
    std::cout << "server name: =>" << server_name[i] << "<=\n";
  }

  std::cout << "root: =>" << root << "<=\n";

  for (size_t i = 0; i < index.size(); i++) {
    std::cout << "index: =>" << index[i] << "<=\n";
  }

  for (std::map<int, std::string>::const_iterator it = error_page.begin();
       it != error_page.end();
       it++) {
    std::cout << "error_page: =>" << it->second << "<=\n";
  }

  std::cout << "timeout: =>" << timeout << "<=\n";

  std::cout << "client_max_body_size: =>" << client_max_body_size << "<=\n";

  std::cout << "access_log: =>" << log["access_log"] << "<=\n";

  std::cout << "error_log: =>" << log["error_log"] << "<=\n";

  std::cout << "autoindex: =>" << autoindex << "<=\n";

  for (std::map<std::string, std::string>::const_iterator it = cgi.begin();
       it != cgi.end();
       it++) {
    std::cout << "cgi: =>" << it->first << "<= =>" << it->second << "<=\n";
  }

  std::cout << "redirect: =>" << redirect.first << "<= =>"
            << redirect.second << "<=\n";

  std::cout << "sockfd: =>" << sockfd << "<=\n";

  for (std::map<std::string, server_location>::const_iterator
           it = location.begin();
       it != location.end();
       it++) {
    std::string index = it->first;

    std::cout << "location name: =>" << index << "<=\n";

    std::cout << "    root: =>" << location[index].root << "<=\n";

    for (size_t i = 0; i < location[index].index.size(); i++) {
      std::cout << "    index: =>" << location[index].index[i] << "<=\n";
    }

    for (size_t i = 0; i < location[index].limit_except.size(); i++) {
      std::cout << "    limit_except: =>"
                << location[index].limit_except[i] << "<=\n";
    }

    std::cout << "    client_max_body_size: =>"
              << location[index].client_max_body_size << "<=\n";

    std::cout << "    autoindex: =>" << location[index].autoindex << "<=\n";

    for (std::map<std::string, std::string>::const_iterator
             it = location[index].cgi.begin();
         it != location[index].cgi.end();
         it++) {
      std::cout << "    cgi: =>" << it->first << "<= =>"
                << it->second << "<=\n";
    }

    std::cout << "    redirect: =>" << location[index].redirect.first
              << "<= =>" << location[index].redirect.second << "<=\n";
  }

  std::cout << "\n";
  n++;
}
