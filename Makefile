all:
	c++ -std=c++11 example.cpp -o Example -lsqlite3 -Wall -Wextra

install:
	cp sqlite.hpp /usr/local/include

clean:
	rm Example