XX=g++

all: socks_server.cpp
	g++ socks_server.cpp -o socks_server -I /usr/local/include -L /usr/local/lib -std=c++17 -Wall -pedantic -pthread -lboost_system -lboost_regex
clean:
	rm -f socks_server
