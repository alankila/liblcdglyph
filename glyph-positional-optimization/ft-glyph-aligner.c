#include <ft2build.h>
#include <freetype/freetype.h>
#include <freetype/ftglyph.h>
#include <stdio.h>
#include <png.h>

#define WIDTH 800

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

    int height = size_in_px * 2;

    const char *text = "The quick brown fox jumps over the lazy fog. äöå $€#! @ 0123456789+-/\\*[](){}";
    int textlen = strlen(text);
    uint32_t *picture = calloc(sizeof(int), WIDTH * height);

    int pos = 0;
    for (int i = 0; i < textlen; i += 1) {
        char currentchar = text[i];
        int glyph_index = FT_Get_Char_Index(face, currentchar);

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

        FT_Bitmap bitmap = face->glyph->bitmap;
        for (int y = 0; y < bitmap.rows; y ++) {
            if (y >= height) {
                break;
            }
            for (int x = 0; x < bitmap.width; x ++) {
                uint8_t c = bitmap.buffer[y * bitmap.pitch + x];
                int pixel = (pos + x) / 3;
                int subpixel = (pos + x) % 3;
                if (pixel >= WIDTH) {
                    break;
                }
         
                uint32_t mask = 0xfff ^ (0xf00 >> (subpixel * 8));
                uint32_t v = picture[pixel + WIDTH * y] & mask;
                v += c;
                if (v > 255) {
                    v = 255;
                }
                picture[pixel + WIDTH * y] &= 0xfff ^ mask;
                picture[pixel + WIDTH * y] |= v;
            }
        }

        pos += face->glyph->advance.x * 3 >> 6;
    }

    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop info_ptr = png_create_info_struct(png_ptr);
    png_init_io(png_ptr, stdout);
    
    png_set_IHDR(png_ptr, info_ptr, 800, height,
                 8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    png_write_info(png_ptr, info_ptr);

    png_bytep *row_pointers = (png_bytep *) malloc(sizeof(png_bytep) * height);
    for (int y = 0; y < height; y += 1) {
        row_pointers[y] = (png_byte *) malloc(png_get_rowbytes(png_ptr, info_ptr));
        for (int x = 0; x < WIDTH; x ++) {
            row_pointers[y][x * 3 + 0] = (uint8_t) picture[y * WIDTH + x] >> 16;
            row_pointers[y][x * 3 + 1] = (uint8_t) picture[y * WIDTH + x] >> 8;
            row_pointers[y][x * 3 + 2] = (uint8_t) picture[y * WIDTH + x] >> 0;
        }
    }

    png_write_image(png_ptr, row_pointers);
    png_write_end(png_ptr, NULL);
}
