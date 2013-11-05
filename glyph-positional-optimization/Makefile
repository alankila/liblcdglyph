CFLAGS = -O2 -Wall -std=c99 -I/usr/local/include/freetype2
LDFLAGS = -lfreetype -lpng

ft-glyph-aligner: ft-glyph-aligner.o
	gcc -o $@ $< $(LDFLAGS)
