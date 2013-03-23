#include <lcdglyph.h>
#include <stdint.h>
#include <stdio.h>

uint8_t bitmap[100] = {
    255, 255,   0,   0,   0,   0,   0,   0,   0, 255,
      0, 255, 255,   0,   0,   0,   0,   0, 128, 128,
      0,   0, 255, 255,   0,   0,   0,   0, 255,   0,
      0,   0,   0, 255, 255,   0,   0, 128, 128,   0,
      0,   0,   0,   0, 255, 255,   0, 255,   0,   0,
      0,   0,   0,   0,   0, 255, 255, 128,   0,   0,
      0,   0,   0,   0,   0,   0, 255, 255,   0,   0,
      0,   0,   0,   0,   0, 128, 128, 255, 255,   0,
      0,   0,   0,   0,   0, 255,   0,   0, 255, 255,
      0,   0,   0,   0, 128, 128,   0,   0,   0, 255,
};

uint8_t bitmap_read(lcdg_reader_t *reader, int32_t x, int32_t y) {
    uint8_t *data = reader->data;
    return data[y * reader->stride + x];
}

uint32_t target[4 * 10] = {};

void target_write(lcdg_writer_t *writer, int32_t x, int32_t y, uint8_t value) {
    uint32_t *data = writer->data;
    int32_t shift = (2 - (x % 3)) * 8;
    data[y * writer->stride + x / 3] |= value << shift;
}

/* Print something for gnuplot */
int
main(int argc, char **argv)
{
    uint8_t *table = lcdg_get_default_table();

    lcdg_reader_t reader = { 
	.width = 10,
	.height = 10,
	.stride = 10,
	.data = bitmap,
	.read = bitmap_read
    };
    lcdg_writer_t writer = {
	.width = 12,
	.height = 10,
	.stride = 4,
	.data = target,
	.write = target_write
    };

    uint8_t *tables[3] = { table, table, table };
    uint8_t color[3] = { 255, 255, 255 };

    lcdg_filter_horizontal(&reader, &writer, tables, color, 128);

    for (int i = 0; i < writer.height; i ++) {
	for (int j = 0; j < reader.width / 3; j ++) {
	    uint32_t *data = (uint32_t *) writer.data;
	    uint32_t value = data[i * writer.stride + j];
	    fprintf(stdout, "%3d %3d %3d ", (value & 0xff0000) >> 16, (value & 0xff00) >> 8, value & 0xff);
	}
        fprintf(stdout, "\n");
    }
}
