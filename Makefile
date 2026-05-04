CXX = g++
CXX_STD = -std=c++11

all:
	$(CXX) $(CXX_STD) example.cpp -o Example -lsqlite3 -Wall -Wextra -Werror -pedantic

install:
	cp sqlite.hpp /usr/include

clean:
	rm Example example.db backup.db
