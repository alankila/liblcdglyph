CFLAGS = -O2 -std=c99 -I/usr/include/freetype2
LDFLAGS = -lfreetype

ft-glyph-aligner: ft-glyph-aligner.o
	gcc -o $@ $< $(LDFLAGS)

