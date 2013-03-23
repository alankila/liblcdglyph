/* 
 * (c) 2013 Antti S. Lankila / BEL Solutions Oy
 * See COPYING for the applicable Open Source license.
 *
 * Perform filtering of glyphs based on input parameters.
 */

#include <stdint.h>

#include "lcdglyph.h"

static uint8_t
enhance(uint8_t enhancement,
	uint8_t component,
	uint8_t foreground)
{
    uint8_t contrast = (255 - foreground) * enhancement >> 8;
    return component + (contrast * component * (255 - component) >> 16);
}

static uint8_t
filter(uint8_t a1,
       uint8_t a2,
       uint8_t a3)
{
    return (0x55 * a1 + 0x56 * a2 + 0x55 * a3 + (1 << 7)) >> 8;
}

void
lcdg_filter_horizontal(lcdg_reader_t *reader,
	    	       lcdg_writer_t *writer,
	               uint8_t *alpha_correct_table[3],
	   	       uint8_t foreground[3],
	    	       uint8_t dark_glyph_enhancement)
{
    for (int32_t y = 0; y < reader->height && y < writer->height; y ++) {
	uint8_t a1 = 0;
	uint8_t a2 = 0;
	uint8_t a3 = 0;

	/* Start from blue component because x = -1 initially */
	uint8_t fgidx = 2;

	/* 1 pixel extra space on both sides to spill LCD filtering */
	for (int32_t x = -1; x < reader->width + 1 && x + 1 < writer->width; x ++) {
	    /* Read pixel from input bitmap, or 0 if access falls outside bound.
	     * a2 represents current subpixel, a1 previous, and a3 next. */
	    a1 = a2;
	    a2 = a3;
	    a3 = x + 1 < reader->width ? reader->read(reader, x + 1, y) : 0;

	    /* Determine foreground color */
	    uint8_t fg = foreground[fgidx];

	    a3 = enhance(dark_glyph_enhancement, a3, fg);
	    uint8_t a = filter(a1, a2, a3);
	    uint8_t ac = alpha_correct_table[fgidx][fg << 8 | a];

	    writer->write(writer, x + 1, y, ac);
	    
	    if (++ fgidx == 3) {
		fgidx = 0;
	    }
	}
    }
}
