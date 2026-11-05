all: caller_test callee_test

caller_test: caller_test.cc
	g++ -g -std=c++17 -o $@ $^ -lgtest -lgtest_main -pthread

callee_test: callee_test.cc
	g++ -g -std=c++17 -o $@ $^ -lgtest -lgtest_main -pthread

.PHONY: all clean
clean:
	rm -f caller_test callee_test
