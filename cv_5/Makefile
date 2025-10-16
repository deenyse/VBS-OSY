CC = gcc
CFLAGS = -Wall -Wextra
LDFLAGS = -lm  

TARGETS = main gen filter transform sum

all: $(TARGETS)

calcpipe: main.c
	$(CC) $(CFLAGS) -o $@ $<

gen: gen.c
	$(CC) $(CFLAGS) -o $@ $<

filter: filter.c
	$(CC) $(CFLAGS) -o $@ $<

transform: transform.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

sum: sum.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS)
