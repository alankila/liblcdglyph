#include <ft2build.h>
#include <freetype/freetype.h>
#include <freetype/ftglyph.h>
#include <stdio.h>

void draw_glyph_bitmap(FT_Bitmap bitmap) {
    for (int y = 0; y < bitmap.rows; y ++) {
        for (int x = 0; x < bitmap.width; x ++) {
            unsigned char c = bitmap.buffer[y * bitmap.pitch + x];
	    if (c == 0) {
	        fprintf(stdout, "   ");
	    } else {
	        fprintf(stdout, "%02x ", c);
	    }
        }
        fprintf(stdout, "\n");
    }
}

/* The higher score the better. Can be negative. Absolute value has no meaning. */
int score_glyph_bitmap(FT_Bitmap bitmap) {
    int score = 0;
    for (int y = 0; y < bitmap.rows; y ++) {
        for (int x = 0; x < bitmap.width; x ++) {
            unsigned char c = bitmap.buffer[y * bitmap.pitch + x];
	    score -= c * (255 - c);
        }
    }
    return score;
}

/*
int score_glyph(FT_GlyphSlot glyph, int dx, int dy) {
    FT_Glyph copy;
    FT_Error error;

    error = FT_Get_Glyph(glyph, &copy);
    FT_Vector position = { dx, dy };
    error = FT_Glyph_To_Bitmap(&copy, FT_RENDER_MODE_NORMAL, &position, 1);
    if (error) {
        fprintf(stderr, "FT glyph to bitmap: error %d\n", error);
        return -0x7ffffff;
    }
    FT_BitmapGlyph bitmapcopy = (FT_BitmapGlyph) copy;

    int score = score_glyph_bitmap(bitmapcopy->bitmap);
    FT_Done_Glyph(copy);
    return score;
}*/

/*int draw_glyph(FT_GlyphSlot glyph, int dx, int dy) {
    FT_Glyph copy;
    FT_Error error;

    error = FT_Get_Glyph(glyph, &copy);
    FT_Vector position = { dx, dy };
    error = FT_Glyph_To_Bitmap(&copy, FT_RENDER_MODE_NORMAL, &position, 1);
    if (error) {
        fprintf(stderr, "FT glyph to bitmap: error %d\n", error);
        return 0;
    }
    FT_BitmapGlyph bitmapcopy = (FT_BitmapGlyph) copy;

    int score = score_glyph_bitmap(bitmapcopy->bitmap);
    fprintf(stdout, "Translation: (%d %d) px/64; Baseline: (%d %d) px; Size (%d %d) px; Score: %d\n", dx, dy, bitmapcopy->left, bitmapcopy->top, bitmapcopy->bitmap.width, bitmapcopy->bitmap.rows, score);
    draw_glyph_bitmap(bitmapcopy->bitmap);
    FT_Done_Glyph(copy);
    fprintf(stdout, "\n");
    return score;
}
*/

int main(int argc, char **argv) {
    if (argc != 3) {
	fprintf(stderr, "Usage: %s font_file size_in_px\n", argv[0]);
	return 1;
    }

    char *font_name = argv[1];
    int size_in_px = atoi(argv[2]);

    FT_Library library;
    FT_Error error;

    error = FT_Init_FreeType(&library);
    if (error) {
	fprintf(stderr, "FT init freetype: error %d\n", error);
	return 1;
    }

    FT_Face face;
    error = FT_New_Face(library, font_name, 0, &face);
    if (error) {
	fprintf(stderr, "FT new face: error %d\n", error);
	return 1;
    }

    error = FT_Set_Pixel_Sizes(face, size_in_px, size_in_px);
    if (error) {
	fprintf(stderr, "FT set pixel sizes: error %d\n", error);
	return 1;
    }

    /* Scan for optimal y offset */
    int bestdy = 0;
    int bestscore = -0x7fffffff;
    int defaultscore = 0;
    for (int dy = -32; dy < 32; dy ++) {
        FT_Vector position = { 0, dy };
        FT_Set_Transform(face, 0, &position);
        int score = 0;
        for (int glyph_index = 1; glyph_index < face->num_glyphs; glyph_index ++) {
	    error = FT_Load_Glyph(face, glyph_index, FT_LOAD_NO_AUTOHINT | FT_LOAD_NO_HINTING | FT_LOAD_NO_BITMAP);
            if (error) {
                fprintf(stderr, "FT load glyph: error %d\n", error);
                return 1;
            }
	    error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
	    if (error) {
                fprintf(stderr, "FT render glyph: error %d\n", error);
                return 1;
	    }
            score += score_glyph_bitmap(face->glyph->bitmap);
        }

	if (dy == 0) {
	    defaultscore = score;
	}

        if (score > bestscore) {
            bestscore = score;
	    bestdy = dy;
        }
    }

    fprintf(stdout, "Total change: %d -> %d with dy %d\n", defaultscore, bestscore, bestdy);
}
