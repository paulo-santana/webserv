//Copyright (c) 2022 João Rodriguez A.K.A. VLN37. All rights reserved.
//Creation date: 24/06/2022

#include "webserv.h"

int init(char **argv,
                 std::map<int, Server*>* map,
                 pollfd *pollfds) {
  int i = 0;
  struct server_config** configfile = readconfig(argv);

  while (configfile[i]) {
    Server *tmp = new Server;
    tmp->_socket();
    tmp->_bind(configfile[i]->listen);
    tmp->_listen(500);

    map->insert(std::make_pair(tmp->sockfd, tmp));
    pollfds[i].fd = tmp->sockfd;
    pollfds[i].events = POLLIN;
    i++;
  }
  return i;
}
