#ifndef _LCDGLYPH_H
#define _LCDGLYPH_H 1

#include <stdint.h>

uint8_t *lcdg_get_default_table();

void lcdg_build_table(uint8_t *table, uint8_t bg_start, uint8_t bg_end);

typedef uint8_t lcdg_reader_func_t(void *data, uint32_t x, uint32_t y);

typedef struct {
	uint32_t width;
	uint32_t height;
	uint32_t stride;
	void *data;
	lcdg_reader_func_t *read;
} lcdg_reader_t;

typedef uint8_t lcdg_writer_func_t(void *data, uint32_t x, uint32_t y, uint8_t value);

typedef struct {
	uint32_t stride;
	void *data;
	lcdg_writer_func_t *write;
} lcdg_writer_t;

#endif
