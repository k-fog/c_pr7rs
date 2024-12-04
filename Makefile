CC=clang
CFLAGS=-std=c11 -Wall -g

pr7rs: c_pr7rs.c
	$(CC) -o $@ $(CFLAGS) $^

run:
	./pr7rs

clean:
	rm -f *.o *~ pr7rs

test: pr7rs
	./test.sh

.PHONY: run clean test
