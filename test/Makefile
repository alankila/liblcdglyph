CFLAGS = -O2 -std=c99 -Wall -I../src
LDFLAGS = -lm -L../src -llcdglyph

generate_table: generate_table.o
	gcc -o $@ $< $(LDFLAGS)
