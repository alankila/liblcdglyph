/* 
 * (c) 2013 Antti S. Lankila / BEL Solutions Oy
 * See COPYING for the applicable Open Source license.
 *
 * Build alpha correction table where index is fg << 8 | alpha and value is
 * the corrected alpha.
 *
 * Theory of alpha correction is based on the notion that correct sRGB
 * blending can be achieved with linearly blending sRGB operator is the alpha
 * channel may be adjusted freely. Furthermore, it is generally true that text
 * is designed to be read, so we can make the assumption that background is
 * far away from foreground. In particular, we can treat large fg-bg
 * differences as more likely.
 *
 * The problem can be solved generically as an optimization problem where
 * the potential background choices are tried against the result of a trial
 * alpha correction value, and the value for alpha correction is chosen that
 * yields the lowest error based on some reasonable error metric.
 *
 */
#include <math.h>
#include <stdint.h>

#include "lcdglyph.h"

static uint16_t s2l[65536];
static uint16_t l2s[65536];

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

static void
init()
{
    static uint8_t inited = 0;
    if (inited) {
	return;
    }
    inited = 1;

    for (int32_t c = 0; c < 65536; c ++) {
	s2l[c] = roundf(srgb_to_linear(c / 65535.0f) * 65535.0f);
    }

    for (int32_t c = 0; c < 65536; c ++) {
	l2s[c] = roundf(linear_to_srgb(c / 65535.0f) * 65535.0f);
    }
}

void
lcdg_build_table(uint8_t *table,
		 float *error,
		 uint8_t startbg_,
		 uint8_t endbg_)
{
    init();

    int32_t startbg = startbg_ * 0x101;
    int32_t endbg = (endbg_ + 1) * 0x101;

    for (int32_t fg = 0; fg < 65536; fg += 0x101) {
	int32_t startac = 0;
	for (int32_t a = 0; a < 256; a ++) {
	    uint32_t besterror = 0xffffffff;
	    int32_t bestac = 0;

	    /* find the best ac for each (alpha, fg) pair.
	     * f(alpha) = ac appears to be monotonic,
	     * so we start search from the previous value. */
	    for (int32_t ac = startac; ac < 256; ac ++) {
		uint32_t error = 0;
		for (int32_t bg = startbg; bg < endbg; bg += 0x101) {
		    int32_t linear_blended = (ac * fg + (255 - ac) * bg + 128) / 255;
		    int32_t srgb_blended = l2s[(a * s2l[fg] + (255 - a) * s2l[bg] + 128) / 255];

		    /* 12 bits, should suffice to represent the linearly coded value exactly enough */
		    int32_t difference = (linear_blended - srgb_blended) >> 4;
		    /* 24 bits, 32 sums */
		    error += difference * difference;
		}

		/* We assume that the error function is generally U-shaped,
		 * so we terminate search once it seems we start the latter leg of U */
		if (error <= besterror) {
		    besterror = error;
		    bestac = ac;
		} else {
		    break;
		}
	    }

	    startac = bestac;
	    if (table != 0) {
		*table = bestac;
		table ++;
	    }
	    if (error != 0) {
		*error = sqrtf(besterror / 256.0f / 256.0f);
		error ++;
	    }
	}
    }
}
