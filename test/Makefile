CFLAGS = -O2 -std=c99 -Wall -I../src
LDFLAGS = -lm -L../src -llcdglyph

.phony: all

all: generate_table filter_bitmap

generate_table: generate_table.o
	gcc -o $@ $< $(LDFLAGS)

filter_bitmap: filter_bitmap.o
	gcc -o $@ $< $(LDFLAGS)

