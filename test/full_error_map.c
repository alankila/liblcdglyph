#include <math.h>
#include <lcdglyph.h>
#include <stdint.h>
#include <stdio.h>

static float
srgb_to_linear(float c)
{
    if (c <= 0.04045f) {
        return c / 12.92f;
    } else {
        return powf((c + 0.055f) / 1.055f, 2.4f);
    }
}

static float
linear_to_srgb(float c)
{
    if (c <= 0.0031308f) {
        return c * 12.92f;
    } else {
        return 1.055f * powf(c, 1.0f/2.4f) - 0.055f;
    }
}

/* Print something for gnuplot */
int
main(int argc, char **argv)
{
    uint16_t s2l[65536];
    uint16_t l2s[65536];
    uint8_t table[65536];

    for (int32_t c = 0; c < 65536; c ++) {
        s2l[c] = roundf(srgb_to_linear(c / 65535.0f) * 65535.0f);
    }

    for (int32_t c = 0; c < 65536; c ++) {
        l2s[c] = roundf(linear_to_srgb(c / 65535.0f) * 65535.0f);
    }

    lcdg_build_table(table, 0, 0, 255);
    int32_t alpha = 128;
    for (int32_t i = 0; i < 65536; i ++) {
	int32_t fg = i >> 8;
	int32_t bg = i & 0xff;
	int32_t ac = table[fg << 8 | alpha];

	int32_t correct = l2s[(s2l[fg * 0x101] * alpha + s2l[bg * 0x101] * (255 - alpha) + 128) / 255];
	int32_t approximated = (fg * 0x101 * ac + bg * 0x101 * (255 - ac) + 128) / 255;
	int32_t broken = (fg * 0x101 * alpha + bg * 0x101 * (255 - alpha) + 128) / 255;

	int32_t error_approximated = approximated - correct;
	if (error_approximated < 0) {
	    error_approximated = -error_approximated;
	}
	int32_t error_broken = broken - correct;
	if (error_broken < 0) {
	    error_broken = -error_broken;
	}

	fprintf(stdout, "%d %d %f\n", fg, bg, (error_broken - error_approximated) / 257.0f);
	if (bg == 255) {
	    fprintf(stdout, "\n");
	}
    }
}
