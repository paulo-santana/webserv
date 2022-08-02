CC      = c++
CFLAGS  = -g3 -Wall -Wextra -Werror -std=c++98 -pedantic
CFLAGS  += -MMD -MP

INCPATH = -I./sources
NAME    = webserv

SRC     = main.cpp \
          WebServ.cpp \
          ResponseBase.cpp \
          Request.cpp \
          RequestParser.cpp \
          Pollfd.cpp \
          Server.cpp \
          ServerLocation.cpp \
          Config.cpp \
          Logger.cpp \
          Response.cpp \
          LoadException.cpp \
          validate_input.cpp \
          signal.cpp \
          String.cpp \
          ConfigHelper.cpp \


INC     = defines.hpp \
          WebServ.hpp \
          ResponseBase.hpp \
          Request.hpp \
          RequestParser.hpp \
          Pollfd.hpp \
          Server.hpp \
          ServerLocation.hpp \
          Config.hpp \
          Logger.hpp \
          Response.hpp \
          LoadException.hpp \
          validate_input.hpp \
          signal.hpp \
          String.hpp \
          ConfigHelper.hpp \

OBJDIR  = objects
SRC_DIR =	sources
OBJ     = $(SRC:%.cpp=$(OBJDIR)/%.o)
DEPS    = $(SRC:%.cpp=$(OBJDIR)/%.d)

vpath %.cpp sources
vpath %.hpp sources
vpath %.h   sources

all: $(NAME)

$(NAME): $(OBJDIR) $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(NAME)

$(OBJDIR)/%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@ $(INCPATH)

clean:
	rm -rf $(OBJDIR)

fclean: clean
	rm -rf $(NAME)

$(OBJDIR):
	mkdir -p objects

re: fclean all

run: $(NAME)
	./$(NAME) ./default.conf

.PHONY: all clean fclean re

-include $(DEPS)
