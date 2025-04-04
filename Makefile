SHELL := /bin/bash

NAME    = ircserv
SRC     = src/main.cpp src/Server.cpp src/Command.cpp src/Channel.cpp src/Client.cpp src/IRCUtils.cpp
OBJ     = $(SRC:.cpp=.o)
CXX     = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -Iinclude

# Couleurs ANSI pour embellir l'affichage
GREEN   = \033[0;32m
YELLOW  = \033[0;33m
RESET   = \033[0m

# Macro spinner avec barre de chargement
define spinner
	( \
	  command="$1"; \
	  progress=0; \
	  spin_width=50; \
	  $$command & pid=$$!; \
	  while kill -0 $$pid 2>/dev/null; do \
	    progress=$$(( (progress+1) % (spin_width+1) )); \
	    bar="["; \
	    for ((i=0; i<progress; i++)); do \
	      bar="$$bar#"; \
	    done; \
	    for ((i=progress; i<spin_width; i++)); do \
	      bar="$$bar "; \
	    done; \
	    bar="$$bar]"; \
	    printf "\r$(YELLOW)Loading %s$(RESET)" "$$bar"; \
	    sleep 0.1; \
	  done; \
	  wait $$pid; \
	  printf "\r$(GREEN)Done!%*s$(RESET)\n" $$((spin_width+10)) ""; \
	)
endef

all: $(NAME)

$(NAME): $(OBJ)
	@echo "Linking $(NAME)..."
	@$(call spinner, $(CXX) $(CXXFLAGS) -o $(NAME) $(OBJ))

%.o: %.cpp
	@echo "Compiling $<..."
	@$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	@rm -f $(OBJ)
	@echo -e "$(GREEN)Cleaned object files.$(RESET)"

fclean: clean
	@rm -f $(NAME)
	@echo -e "$(GREEN)Cleaned binary $(NAME).$(RESET)"

re: fclean all

.PHONY: all clean fclean re
