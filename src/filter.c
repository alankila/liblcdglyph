/* 
 * (c) 2013 Antti S. Lankila / BEL Solutions Oy
 * See COPYING for the applicable Open Source license.
 *
 * Perform filtering of glyphs based on input parameters.
 */

#include <stdint.h>

#include "lcdglyph.h"

void
lcdg_filter_horizontal(lcdg_reader_t *reader,
	    	       lcdg_writer_t *writer,
	               uint8_t *alpha_correct_table[3],
	   	       uint8_t foreground[3],
	    	       uint8_t dark_glyph_enhancement)
{
    for (int32_t y = 0; y < reader->height && y < writer->height; y ++) {
	/* 1 pixel extra space on both sides to spill LCD filtering */
	for (int32_t x = 0; x < reader->width && x < writer->width; x ++) {
	    uint8_t a = reader->read(reader, x, y);
	    uint8_t fg = foreground[x % 3];
	    uint8_t ac = alpha_correct_table[x % 3][fg << 8 | a];
	    writer->write(writer, x + 1, y, ac);
	}
    }
}
