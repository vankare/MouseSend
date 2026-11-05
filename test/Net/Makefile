all: util_test server_test server client

util_test: util_test.cc
	g++ -g -std=c++17 -o $@ $^ -lgtest -lgtest_main -pthread

server_test: server_test.cc
	g++ -g -std=c++17 -o $@ $^ -lgtest -lgtest_main -pthread

server: server.cc
	g++ -g -std=c++17 -o $@ $^ -pthread

client: client.cc
	g++ -g -std=c++17 -o $@ $^ -pthread

.PHONY: all clean
clean:
	rm -f util_test server_test server client
