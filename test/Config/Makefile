CXXFLAGS := -g -std=c++17
LDLIBS   := -lgtest -lgtest_main -pthread

all: config_test

%_test: %_test.cc
	@$(CXX) $(CXXFLAGS) -o $@ $< $(LDLIBS)

.PHONY: all clean
clean:
	@rm -f config_test
