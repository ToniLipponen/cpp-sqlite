all:
	c++ -std=c++17 example.cpp -o Example -lsqlite3

clean:
	rm Example