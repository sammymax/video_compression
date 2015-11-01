CFLAGS=-Wall -Werror -std=c11 -g
CXXFLAGS=-Wall -Werror -std=c++11 -g
simple-tsan.o:
test1.o: test1.c
	cc -Wall -Werror -std=c11    -c test1.c -fsanitize=thread
test1: test1.o simple-tsan.o
	c++ test1.o simple-tsan.o -o test1
