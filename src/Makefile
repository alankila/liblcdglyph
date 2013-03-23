CFLAGS := -O2 -std=c99 -Wall -fPIC
LDFLAGS := -lm
OBJS := get_default_table.o build_table.o filter.o

liblcdglyph.so: $(OBJS)
	gcc -o $@ -shared $(OBJS) $(LDFLAGS)
