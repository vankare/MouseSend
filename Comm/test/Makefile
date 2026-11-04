all: test server client

test: test.cc
	g++ -g -o $@ $^ -lgtest -lgtest_main -pthread

server: server.cc
	g++ -g -o $@ $^ -pthread

client: client.cc
	g++ -g -o $@ $^ -pthread

.PHONY: clean
clean:
	rm test server client
