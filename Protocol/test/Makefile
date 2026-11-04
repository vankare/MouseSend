all: test test_caller test_callee

test: test.cc 
	g++ -g -o $@ $^ -lgtest -lgtest_main

test_caller: test_caller.cc
	g++ -g -o $@ $^ -lgtest -lgtest_main

test_callee: test_callee.cc
	g++ -g -o $@ $^ -lgtest -lgtest_main

.PHONY: clean
clean:
	rm test test_callee test_caller
