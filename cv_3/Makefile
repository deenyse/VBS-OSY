CC = gcc
CFLAGS = -Wall -Wextra -O2
TARGET = mydiff
SRC = mydiff.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET) *.o

.PHONY: all clean
