#include <lcdglyph.h>
#include <stdint.h>
#include <stdio.h>

/* Print something for gnuplot */
int
main(int argc, char **argv)
{
    uint8_t table[65536];
    float error[65536];
    lcdg_build_table(table, error, 0, 255);
    for (int i = 0; i < 65536; i ++) {
	int fg = i >> 8;
	int alpha = i & 0xff;
        fprintf(stdout, "%d %d %d %f\n", fg, alpha, table[i], error[i]);
	if (alpha == 255) {
	    fprintf(stdout, "\n");
	}
    }
}
