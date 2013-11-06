#include <ft2build.h>
#include <freetype/freetype.h>
#include <freetype/ftglyph.h>
#include <freetype/ftmodapi.h>
#include <freetype/ftoutln.h>
#include <math.h>
#include <png.h>
#include <stdio.h>

#define WIDTH 800

static uint8_t map(float fg, float bg, uint8_t alpha) {
    float a = alpha / 255.0f;
    float mix = fg * a + (1.0f - a) * bg;
    return roundf(powf(mix, 1.0f/2.2f) * 255.0f);
}

static void build_glyph(FT_Face face, int32_t glyph_index) {
    FT_Load_Glyph(face, glyph_index, FT_LOAD_NO_AUTOHINT | FT_LOAD_NO_HINTING | FT_LOAD_NO_BITMAP);
    FT_Outline_EmboldenXY(&face->glyph->outline, 32, 32);
    FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
}

/* The higher the score the better. Can be negative. Absolute value has no meaning.
 * Score is based on minimizing the number of mid-level antialiased pixels. */
static int32_t score_glyph_bitmap(FT_Bitmap bitmap) {
    int32_t score = 0;
    for (int32_t y = 0; y < bitmap.rows; y ++) {
        for (int32_t x = 0; x < bitmap.width; x ++) {
            uint8_t c = bitmap.buffer[y * bitmap.pitch + x];
            score -= c * (255 - c);
        }
    }
    return score;
}

static int32_t scan_optimal_x_offset(FT_Face face) {
    int32_t best = 0;
    int32_t bestscore = -0x7fffffff;
    for (int32_t opt = -32; opt < 32; opt ++) {
        FT_Vector position = { opt, 0 };
        FT_Set_Transform(face, 0, &position);
        int32_t score = 0;
        for (int32_t glyph_index = 1; glyph_index < face->num_glyphs; glyph_index ++) {
            build_glyph(face, glyph_index);
            score += score_glyph_bitmap(face->glyph->bitmap);
        }
        if (score > bestscore) {
            bestscore = score;
            best = opt;
        }
    }
    return best;
}

static int32_t scan_optimal_y_offset(FT_Face face) {
    int32_t best = 0;
    int32_t bestscore = -0x7fffffff;
    for (int32_t opt = -32; opt < 32; opt ++) {
        FT_Vector position = { 0, opt };
        FT_Set_Transform(face, 0, &position);
        int32_t score = 0;
        for (int32_t glyph_index = 1; glyph_index < face->num_glyphs; glyph_index ++) {
            build_glyph(face, glyph_index);
            score += score_glyph_bitmap(face->glyph->bitmap);
        }
        if (score > bestscore) {
            bestscore = score;
            best = opt;
        }
    }

    return best;
}

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s font_file size_in_px color\n", argv[0]);
        return 1;
    }

    char *font_name = argv[1];
    int32_t size_in_px = atoi(argv[2]);
    int32_t color = atoi(argv[3]);

    FT_Library library;
    FT_Error error;

    error = FT_Init_FreeType(&library);
    if (error) {
        fprintf(stderr, "FT init freetype: error %d\n", error);
        return 1;
    }

    /* Disable stem darkening; we have our own thing with bolding */
    FT_Bool no_stem_darkening = 1;
    FT_Property_Set(library, "cff", "no-stem-darkening", &no_stem_darkening);

    FT_Face face;
    error = FT_New_Face(library, font_name, 0, &face);
    if (error) {
        fprintf(stderr, "FT new face: error %d\n", error);
        return 1;
    }

    error = FT_Set_Pixel_Sizes(face, size_in_px*3, size_in_px);
    if (error) {
        fprintf(stderr, "FT set pixel sizes: error %d\n", error);
        return 1;
    }

    int32_t dx = scan_optimal_x_offset(face);
    int32_t dy = scan_optimal_y_offset(face);
    fprintf(stderr, "Translating font face by (%d, %d) 1/64th pixels\n", dx, dy);
    FT_Vector position = { dx, dy };
    FT_Set_Transform(face, 0, &position);

    int32_t height = size_in_px * 2;

    const char *text = "The quick brown fox jumps over the lazy dog. Ta To iiiillll1111|||||////\\\\\\\\";
    int32_t textlen = strlen(text);
    uint8_t *picture = malloc(3 * WIDTH * height);
    for (int32_t i = 0; i < 3 * WIDTH * height; i += 1) {
        picture[i] = 0;
    }

    int32_t pen_x = 0;
    int32_t pen_y = height / 2;
    int32_t previous = 0;
    for (int i = 0; i < textlen; i += 1) {
        uint8_t currentchar = text[i];
        int32_t glyph_index = FT_Get_Char_Index(face, currentchar);
        build_glyph(face, glyph_index);

        if (previous) {
            FT_Vector delta;
            FT_Get_Kerning(face, previous, glyph_index, FT_KERNING_UNFITTED, &delta);
            pen_x += (delta.x + 32) >> 6;
        }
        previous = glyph_index;

        FT_Bitmap bitmap = face->glyph->bitmap;
        for (int32_t y = 0; y < bitmap.rows; y ++) {
            int32_t pos_y = y + pen_y - face->glyph->bitmap_top;
            if (pos_y < 0 || pos_y >= height) {
                continue;
            }

            for (int32_t x = 0; x < bitmap.width; x ++) {
                int32_t c = bitmap.buffer[y * bitmap.pitch + x];
 
                int32_t fir[5] = { 0x0, 0x55, 0x55, 0x55, 0x0 };
                for (int32_t dx = -2; dx <= 2; dx ++) {
                    int32_t pos_x = dx + x + pen_x + face->glyph->bitmap_left;
                    if (pos_x < 0 || pos_x >= WIDTH * 3) {
                        continue;
                    }

                    int32_t x = picture[pos_x + WIDTH * 3 * pos_y];
                    x += (c * fir[dx+2] + 128) >> 8;
                    if (x > 255) {
                        fprintf(stderr, "overflow at (%d, %d)\n", pos_x, pos_y);
                        x = 255;
                    }
                    picture[pos_x + WIDTH * 3 * pos_y] = x;
                }
            }
        }

        pen_x += (face->glyph->advance.x + 32) >> 6;
    }

    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop info_ptr = png_create_info_struct(png_ptr);
    png_init_io(png_ptr, stdout);
    
    png_set_IHDR(png_ptr, info_ptr, WIDTH, height,
                 8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    png_set_sRGB(png_ptr, info_ptr, PNG_sRGB_INTENT_SATURATION);

    png_write_info(png_ptr, info_ptr);

    png_bytep *row_pointers = (png_bytep *) malloc(sizeof(png_bytep) * height);
    for (int32_t y = 0; y < height; y += 1) {
        row_pointers[y] = (png_byte *) malloc(png_get_rowbytes(png_ptr, info_ptr));
        for (int32_t x = 0; x < WIDTH; x += 1) {
            switch (color) {
            case 0:
                row_pointers[y][x*4+0] = map(0, 1, picture[y * WIDTH * 3 + x*3+0]);
                row_pointers[y][x*4+1] = map(0, 1, picture[y * WIDTH * 3 + x*3+1]);
                row_pointers[y][x*4+2] = map(0, 1, picture[y * WIDTH * 3 + x*3+2]);
                break;
            case 1:
                row_pointers[y][x*4+0] = map(1, 0, picture[y * WIDTH * 3 + x*3+0]);
                row_pointers[y][x*4+1] = map(1, 0, picture[y * WIDTH * 3 + x*3+1]);
                row_pointers[y][x*4+2] = map(1, 0, picture[y * WIDTH * 3 + x*3+2]);
                break;
            case 2:
                row_pointers[y][x*4+0] = map(1, 0, picture[y * WIDTH * 3 + x*3+0]);
                row_pointers[y][x*4+1] = map(0, 1, picture[y * WIDTH * 3 + x*3+1]);
                row_pointers[y][x*4+2] = 0;
                break;
            }
            row_pointers[y][x*4+3] = 0xff;
        }
    }

    png_write_image(png_ptr, row_pointers);
    png_write_end(png_ptr, NULL);
}
