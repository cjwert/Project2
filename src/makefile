OBJS = shell.cpp builtins.cpp
NAME = MyShell

all: $(NAME)

MyShell: $(OBJS)
	g++ -std=c++11 $(OBJS) -l readline -o $(NAME)
	
clean:
	rm -rf $(NAME)