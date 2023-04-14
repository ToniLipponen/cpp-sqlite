all:
	c++ -std=c++11 example.cpp -o Example -lsqlite3 -Wall -Wextra -Werror

install:
	cp sqlite.hpp /usr/include

clean:
	rm Example example.db backup.db
