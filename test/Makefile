CFLAGS = -O2 -std=c99 -Wall -I../src
LDFLAGS = -lm -L../src -llcdglyph

.phony: all

all: generate_table full_error_map

generate_table: generate_table.o
	gcc -o $@ $< $(LDFLAGS)

full_error_map: full_error_map.o
	gcc -o $@ $< $(LDFLAGS)
