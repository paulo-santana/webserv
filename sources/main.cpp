//##############################################################################
//#              Copyright(c)2022 Turbo Development Design (TDD)               #
//#                         João Rodriguez - VLN37                             #
//#                         Paulo Sergio - psergio-                            #
//#                         Welton Leite - wleite                              #
//##############################################################################

#include "WebServ.hpp"

void loop(int argc, char** argv) {
  WebServ webserv;

  webserv.init(argc, argv);
  while (true) {
    webserv._poll();
    if (webserv.conn <= 0)
      break;
    for (int i = 0, size = webserv.pollfds.size(); i < size; i++) {
      int16_t revents = webserv.pollfds[i].revents;
      bool server_request = webserv.serverlist.count(webserv.pollfds[i].fd);
      if (!server_request && webserv.timed_out(i)) {
        webserv.end_connection(i);
        continue;
      }
      if (revents == 0)
        continue;
      if (server_request) {
        webserv._accept(i);
      } else {
        if (revents & POLLERR ||
            revents & POLLRDHUP ||
            revents & POLLNVAL ||
            revents & POLLHUP)
            webserv.end_connection(i);
        else if (webserv.timed_out(i))
              webserv.end_connection(i);
        else if (revents & POLLIN) {
          webserv.log.error() << webserv.pollfds[i].fd << " pollin\n";
          webserv._receive(i);
        }
        else if (revents & POLLOUT) {
          webserv.log.error() << webserv.pollfds[i].fd << " pollout\n";
          webserv._respond(i);
        }
        else
          WebServ::log.warning() << "unexpected error returned on poll";
      }
    }
    if (webserv.compress)
      webserv.purge_conns();
  }
}

int main(int argc, char** argv) {
  while (true) {
    try {
      loop(argc, argv);
      break;
    } catch (LoadException& e) {
      WebServ::log.error() << e.what() << std::endl;
      return (1);
    } catch (std::exception& e) {
      WebServ::log.error() << e.what() << "\n";
      WebServ::log.error() << "EXCEPTION HANDLED REINITIALIZING...\n";
    }
  }
}
