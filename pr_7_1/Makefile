# Makefile for compiling chat server and client (C++)

# === EXECUTABLES ===
SERVER_TARGET = chat_server
CLIENT_TARGET = chat_client

# === SOURCE FILES ===
SERVER_SOURCES = server.cpp
CLIENT_SOURCES = socet.cpp

# === COMPILER ===
CXX = g++

# === COMPILATION FLAGS ===
CXXFLAGS = -Wall -Wextra -std=c++11 -pthread

# === OBJECT FILES ===
SERVER_OBJECTS = $(SERVER_SOURCES:.cpp=.o)
CLIENT_OBJECTS = $(CLIENT_SOURCES:.cpp=.o)

# === DEFAULT: build everything ===
all: $(SERVER_TARGET) $(CLIENT_TARGET)

# === SERVER ===
$(SERVER_TARGET): $(SERVER_OBJECTS)
	$(CXX) $(SERVER_OBJECTS) -o $(SERVER_TARGET)

# === CLIENT ===
$(CLIENT_TARGET): $(CLIENT_OBJECTS)
	$(CXX) $(CLIENT_OBJECTS) -o $(CLIENT_TARGET)

# === COMPILATION: .cpp → .o ===
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# === CLEAN ===
clean:
	rm -f $(SERVER_OBJECTS) $(CLIENT_OBJECTS) $(SERVER_TARGET) $(CLIENT_TARGET)

# === PHONY TARGETS ===
.PHONY: all clean server client