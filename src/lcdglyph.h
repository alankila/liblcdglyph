#ifndef _LCDGLYPH_H
#define _LCDGLYPH_H 1

#include <stdint.h>

uint8_t *lcdg_get_default_table();

void lcdg_build_table(uint8_t *table, float *error, uint8_t bg_start, uint8_t bg_end);

struct lcdg_reader;

typedef uint8_t lcdg_reader_func_t(struct lcdg_reader *reader, int32_t x, int32_t y);

typedef struct lcdg_reader {
	int32_t width;
	int32_t height;
	int32_t stride;
	void *data;
	lcdg_reader_func_t *read;
} lcdg_reader_t;

struct lcdg_writer;

typedef void lcdg_writer_func_t(struct lcdg_writer *writer, int32_t x, int32_t y, uint8_t value);

typedef struct lcdg_writer {
	int32_t width; /* width is in subpixels, i.e multiplied by 3. */
	int32_t height; /* height is in pixel rows */
	int32_t stride;
	void *data;
	lcdg_writer_func_t *write;
} lcdg_writer_t;

void lcdg_filter_horizontal(lcdg_reader_t *reader, lcdg_writer_t *writer, uint8_t *alpha_correct_table[3], uint8_t foreground[3], uint8_t dark_glyph_enhancement);

#endif
