CXX = g++
CXXFLAGS = -std=c++17 -Wall -O2
LDFLAGS = -lrt

all: server client

server: server.cpp
	$(CXX) $(CXXFLAGS) -o server server.cpp $(LDFLAGS)

client: client.cpp
	$(CXX) $(CXXFLAGS) -o client client.cpp

clean:
	rm -f server client
