CFLAGS = -O2 -Wall -std=c99 -I/usr/local/include/freetype2
LDFLAGS = -lfreetype -lpng
SIZES = 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20

.phony: all

all: ft-glyph-aligner test6.png inv6.png rev6.png

ft-glyph-aligner: ft-glyph-aligner.o
	gcc -o $@ $< $(LDFLAGS)

test6.png: ft-glyph-aligner
	for i in $(SIZES); do ./ft-glyph-aligner /System/Library/Fonts/HelveticaNeueDeskUI.ttc $$i 0 > test$$i.png; done

rev6.png: ft-glyph-aligner
	for i in $(SIZES); do ./ft-glyph-aligner /System/Library/Fonts/HelveticaNeueDeskUI.ttc $$i 1 > rev$$i.png; done

inv6.png: ft-glyph-aligner
	for i in $(SIZES); do ./ft-glyph-aligner /System/Library/Fonts/HelveticaNeueDeskUI.ttc $$i 2 > inv$$i.png; done

