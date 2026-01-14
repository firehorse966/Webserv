NAME = webserv
CC = c++
FLAGS = -Wall -Wextra -Werror -std=c++98  #-fsanitize=address -g

SRC = main.cpp Sockets.cpp Logs.cpp Requests.cpp Response.cpp ConfigParser.cpp \
	Methods.cpp

OBJ = $(SRC:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJ)
	$(CC) $(FLAGS) $(OBJ) -o $(NAME)

%.o: %.cpp
	$(CC) $(FLAGS) -c $< -o $@

clean:
	rm -f $(OBJ)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re