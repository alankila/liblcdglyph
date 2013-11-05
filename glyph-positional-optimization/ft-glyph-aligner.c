#include <ft2build.h>
#include <freetype/freetype.h>
#include <freetype/ftglyph.h>
#include <freetype/ftoutln.h>
#include <math.h>
#include <png.h>
#include <stdio.h>

#define WIDTH 800

static uint8_t map(uint8_t alpha) {
    float fg = 0.0f;
    float bg = 1.0f;
    float a = alpha / 255.0f;
    float mix = fg * a + bg * (1.0f - a);
    return round(powf(mix, 1.0f/2.2f) * 255.0f);
}

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

    error = FT_Set_Pixel_Sizes(face, size_in_px*3, size_in_px);
    if (error) {
        fprintf(stderr, "FT set pixel sizes: error %d\n", error);
        return 1;
    }

    int height = size_in_px * 2;

    const char *text = "The quick brown fox jumps over the lazy dog. Ta To iiiillll1111|||||////\\\\\\\\";
    int textlen = strlen(text);
    uint8_t *picture = malloc(3 * WIDTH * height);
    for (int i = 0; i < 3 * WIDTH * height; i ++) {
        picture[i] = 0;
    }

    int pen_x = 0;
    int pen_y = height / 2;
    int previous = 0;
    for (int i = 0; i < textlen; i += 1) {
        char currentchar = text[i];
        int glyph_index = FT_Get_Char_Index(face, currentchar);

        error = FT_Load_Glyph(face, glyph_index, FT_LOAD_NO_AUTOHINT | FT_LOAD_NO_HINTING | FT_LOAD_NO_BITMAP);
        if (error) {
            fprintf(stderr, "FT load glyph: error %d\n", error);
            continue;
        }

        /* Darkening: 1/3th pixel -- enough for most sizes */
        error = FT_Outline_EmboldenXY(&face->glyph->outline, 22, 22);
        if (error) {
            fprintf(stderr, "FT outline embolden: error %d\n", error);
            continue;
        }

        error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
        if (error) {
            fprintf(stderr, "FT render glyph: error %d\n", error);
            continue;
        }

        if (previous) {
            FT_Vector delta;
            FT_Get_Kerning(face, previous, glyph_index, FT_KERNING_UNFITTED, &delta);
            pen_x += (delta.x + 32) >> 6;
        }
        previous = glyph_index;

        FT_Bitmap bitmap = face->glyph->bitmap;
        for (int y = 0; y < bitmap.rows; y ++) {
            for (int x = 0; x < bitmap.width; x ++) {
                uint32_t c = bitmap.buffer[y * bitmap.pitch + x];
 
                int pos_x = x + pen_x + face->glyph->bitmap_left;
                int pos_y = y + pen_y - face->glyph->bitmap_top;

                if (pos_x < 0 || pos_x >= WIDTH * 3) {
                    continue;
                }
                if (pos_y < 0 || pos_y >= height) {
                    continue;
                }
                uint32_t x = picture[pos_x + WIDTH * 3 * pos_y];
                x += c;
                if (x > 255) {
                    x = 255;
                }
                picture[pos_x + WIDTH * 3 * pos_y] = x;
            }
        }

        pen_x += (face->glyph->advance.x + 32) >> 6;
    }

    /* LCD filter. Notice that this filter tends to move the bitmap 1 subpixel to right */
    for (int y = 0; y < height; y += 1) {
        int comp[3] = { 0, 0, 0 };
        for (int x = 0; x < WIDTH * 3; x += 1) {
            comp[x % 3] = picture[y * WIDTH * 3 + x];
            picture[y * WIDTH * 3 + x] = (comp[0] + comp[1] + comp[2] + 2) / 3;
        }
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
    for (int y = 0; y < height; y += 1) {
        row_pointers[y] = (png_byte *) malloc(png_get_rowbytes(png_ptr, info_ptr));
        for (int x = 0; x < WIDTH; x += 1) {
            row_pointers[y][x*4+0] = map(picture[y * WIDTH * 3 + x*3+0]);
            row_pointers[y][x*4+1] = map(picture[y * WIDTH * 3 + x*3+1]);
            row_pointers[y][x*4+2] = map(picture[y * WIDTH * 3 + x*3+2]);
            row_pointers[y][x*4+3] = 0xff;
        }
    }

    png_write_image(png_ptr, row_pointers);
    png_write_end(png_ptr, NULL);
}
