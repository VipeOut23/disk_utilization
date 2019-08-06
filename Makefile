CC ?= gcc
CFLAGS ?= -Wall -Wextra -std=c11 -pthread
ALLCFLAGS = $(CFLAGS) $(shell echo | gcc -xc -E -v - 2>&1 | grep -E '^\s' | sed '1d;s/^\s/ -I/' | tr '\n' ' ') # Explictly include system libraries for cdb
OBJ = diskutil.o

diskutil: $(OBJ)
	$(CC) $(ALLCFLAGS) $(OBJ) -o disk_utilization

run: diskutil
	./diskutil $(ARGS)

clean:
	rm -f $(OBJ) diskutil

%.o: %.c
	$(CC) $(ALLCFLAGS) -c $< -o $@

.PHONY: run clean
