//##############################################################################
//#              Copyright(c)2022 Turbo Development Design (TDD)               #
//#                         João Rodriguez - VLN37                             #
//#                         Paulo Sergio - psergio-                            #
//#                         Welton Leite - wleite                              #
//##############################################################################

#include "Response.hpp"

std::ostream& operator<<(std::ostream& o, Response const& rhs) {
  o << std::setfill(' ') << " [ RESPONSE DUMP ]\n"
    << std::setw(15) << std::left << "method" << " : "
    << rhs.method << "\n"
    << std::setw(15) << std::left << "httpversion" << " : "
    << rhs.httpversion << "\n"
    << std::setw(15) << std::left << "statuscode" << " : "
    << rhs.statuscode << "\n"
    << std::setw(15) << std::left << "statusmsg" << " : "
    << rhs.statusmsg << "\n"
    << std::setw(15) << std::left << "path" << " : "
    << rhs.path << "\n"
    << std::setw(15) << std::left << "root" << " : "
    << rhs.root << "\n"
    << std::setw(15) << std::left << "originalroot" << " : "
    << rhs.originalroot << "\n"
    << std::setw(15) << std::left << "contenttype" << " : "
    << rhs.contenttype << "\n"
    << std::setw(15) << std::left << "filetype" << " : "
    << rhs.filetype << "\n"
    << std::setw(15) << std::left << "response_path" << " : "
    << rhs.response_path << "\n";
  (void)o;
  return o;
}

void Response::_send(int fd) {
  ssize_t bytes;
  bytes = send(fd, ResponseBase::buffer_resp, ResponseBase::size, 0);
  if (bytes == 0 || bytes == -1) {
    WebServ::log.error() << "unable to send response: "
                         << strerror(errno) << "\n";
    finished = true;
  }
  WebServ::log.info() << "Response sent to client " << fd << "\n";
}

int Response::validate_limit_except(void) {
  if (location->limit_except.size()) {
    if (location->limit_except[0] == "ALL")
      return CONTINUE;
    std::vector<std::string>::iterator it = location->limit_except.begin();
    std::vector<std::string>::iterator ite = location->limit_except.end();
    if (std::find(it, ite, method) != location->limit_except.end())
       return CONTINUE;
    return METHOD_NOT_ALLOWED;
  }
  return CONTINUE;
}

int Response::validate_http_version(void) {
  if (req->http_version != "HTTP/1.1")
    return HTTP_VERSION_UNSUPPORTED;
  return CONTINUE;
}

void Response::php_cgi(std::string const& body_path) {
  int fd = open("./tmp", O_CREAT | O_RDWR | O_TRUNC, 0644);
  if (fd == -1)
    throw(std::exception());
  int status;
  int pid = fork();
  if (pid == 0) {
    if (dup2(fd, STDOUT_FILENO) == -1) {
      perror("dup2");
      exit(1);
    }
    // std::cout << body_path.c_str();
    execlp("php-cgi", "-f", "-q", body_path.substr(2).c_str(), NULL);
  }
  waitpid(pid, &status, 0);
  close(fd);
  assemble("./tmp");
  unlink("./tmp");
}

void Response::dispatch(std::string const& body_path) {
  std::string extension;
  std::string tmp(body_path.substr(1));

  if (tmp.find_last_of('.') == std::string::npos)
    extension = "text";
  else
    extension = tmp.substr(tmp.find_last_of('.'));

  // TODO(VLN37) change to dynamic extension
  if (location->cgi.count(extension)) {
    contenttype = "Content-Type: text/html; charset=utf-8\n";
    php_cgi(body_path);
  }
  else if (mimetypes.count(extension)) {
    contenttype = mimetypes[extension];
    assemble(body_path);
  }
  else
    WebServ::log.warning() << extension << " support not yet implemented\n";
}

std::string Response::_itoa(size_t nbr) {
  std::stringstream ss;
  std::string       ret;

  ss << nbr;
  ss >> ret;
  return ret;
}


void Response::set_statuscode(int code) {
  std::stringstream ss;

  statuscode.clear();
  ss << code;
  ss >> statuscode;
  statuscode.push_back(' ');
  if (statuslist.count(response_code))
    statusmsg = statuslist[response_code];
  else
    statusmsg = statuslist[(response_code / 100) * 100];

  if (folder_request) {
    create_directory_listing();
    response_path = DFL_TMPFILE;
  }
  else if (response_code >= BAD_REQUEST) {
    if (server->error_page.count(response_code)) {
      response_path = server->root + "/" + server->error_page[response_code];
    }
    else
      create_error_page();
  }
  else if (response_code >= MOVED_PERMANENTLY) {
    if (server->error_page.count(response_code))
      response_path = server->root + "/" + server->error_page[response_code];
    else
      create_redir_page();
  }
}

void Response::assemble_followup(void) {
  char buf[BUFFER_SIZE];
  size_t body_size;
  std::string str;

  file.read(buf, BUFFER_SIZE);
  body_size = file.gcount();
  if (body_size < BUFFER_SIZE || file.eof())
    finished = true;

  std::memmove(&ResponseBase::buffer_resp[str.size()], buf, body_size);
  ResponseBase::size = body_size;
  ResponseBase::buffer_resp[ResponseBase::size] = '\0';
  // WebServ::log.warning() << "Multipart response only partially implemented\n"
  //                        << "message: \n"
  //                        << ResponseBase::buffer_resp << "\n";
}

void Response::assemble(std::string const& body_path) {
  std::string       body;
  size_t            body_size = 0;

  WebServ::log.debug() << *this;
  WebServ::log.debug() << "File requested: " << path << "\n";
  file.open(body_path.c_str(), file.ate);
  body_max_size = file.tellg();
  if (file.bad() || file.fail())
    WebServ::log.error() << "file opening in Response::assemble\n";
  file.seekg(std::ios::beg);
  char buf[BUFFER_SIZE];
  file.read(buf, BUFFER_SIZE);
  body_size = file.gcount();
  if (body_max_size < BUFFER_SIZE) {
    finished = true;
  }
  else {
    // contenttype = "Content-Type: application/octet-stream\n";
    inprogress = true;
  }
  std::string str = httpversion +
                    statuscode +
                    statusmsg +
                    contenttype +
                    DFL_CONTENTLEN;
  str.replace(str.find("LENGTH"), 6, _itoa(body_max_size));
  std::memmove(ResponseBase::buffer_resp, str.c_str(), str.size());
  std::memmove(&ResponseBase::buffer_resp[str.size()], buf, body_size);
  ResponseBase::size = str.size() + body_size;
  ResponseBase::buffer_resp[ResponseBase::size] = '\0';
  // WebServ::log.debug() << ResponseBase::buffer_resp;
}

void Response::process(void) {
  for (size_t i = 0; i < validation_functions.size() && response_code == 0; i++)
  response_code = (this->*validation_functions[i])();
  if (response_code == 0)
    response_code = (this->*method_map[method])();
  set_statuscode(response_code);
  dispatch(response_path);
  if (remove_tmp)
    unlink(DFL_TMPFILE);
}

#include "Response_static.tpp"
#include "Response_constructors.tpp"
#include "Response_delete.tpp"
#include "Response_get.tpp"
#include "Response_post.tpp"
#include "Response_dynamichtml.tpp"