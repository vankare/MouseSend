CXXFLAGS := -g -std=c++17
LDLIBS   := -lgtest -lgtest_main -pthread

all: caller_test callee_test signal_test

%_test: %_test.cc
	@$(CXX) $(CXXFLAGS) -o $@ $< $(LDLIBS)

.PHONY: all clean
clean:
	@rm -f caller_test callee_test signal_test
