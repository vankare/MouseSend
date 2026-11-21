all: message_test

message_test: message_test.cc
	g++ -g -std=c++17 -o $@ $^ -lgtest -lgtest_main -pthread

.PHONY: all clean
clean:
	rm -f message_test
