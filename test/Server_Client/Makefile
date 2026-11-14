CXXFLAGS := -g -std=c++17
LDLIBS   := -lgtest -lgtest_main -pthread

all: server_test client_test

server_test: ../../Server_Client/Server.cc
	@$(CXX) $(CXXFLAGS) -o $@ $< $(LDLIBS)

client_test: ../../Server_Client/Client.cc
	@$(CXX) $(CXXFLAGS) -o $@ $< $(LDLIBS)

.PHONY: all clean
clean:
	@rm -f server_test client_test
