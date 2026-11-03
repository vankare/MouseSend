all: test

test: test.cc
	g++ -g -o $@ $^ -lgtest -lgtest_main -pthread

.PHONY: clean
clean:
	rm test
