all: client server

client: client.cc
	g++ -g -o $@ $^

server: server.cc
	g++ -g -o $@ $^

.PHONY: clean
clean:
	rm client server
