CFLAGS = -O2 -std=c99 -Wall -fPIC
LDFLAGS = -lm

liblcdfilter.so: actable.o build_actable.o glyphfilter.o
	gcc -o $@ -shared $< $(LDFLAGS)
