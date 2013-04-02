#ifndef _LCDGLYPH_H
#define _LCDGLYPH_H 1

#include <stdint.h>

uint8_t *lcdg_get_default_table();

void lcdg_build_table(uint8_t *table, float *error, uint8_t bg_start, uint8_t bg_end);

#endif
