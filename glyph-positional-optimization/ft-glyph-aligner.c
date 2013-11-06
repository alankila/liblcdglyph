#include <ft2build.h>
#include <freetype/freetype.h>
#include <freetype/ftglyph.h>
#include <freetype/ftoutln.h>
#include <math.h>
#include <png.h>
#include <stdio.h>

#define WIDTH 800

/* black on white */
static uint8_t ac_fg0[256] = {
0,0,1,1,2,2,3,3,4,4,5,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13,13,14,14,15,15,16,16,17,17,18,18,19,19,20,20,21,21,22,22,23,23,24,24,25,25,26,27,27,28,28,29,29,30,30,31,31,32,32,33,33,34,34,35,36,36,37,37,38,38,39,39,40,41,41,42,42,43,43,44,45,45,46,46,47,47,48,49,49,50,50,51,52,52,53,53,54,55,55,56,57,57,58,58,59,60,60,61,62,62,63,64,64,65,66,66,67,67,68,69,70,70,71,72,72,73,74,74,75,76,76,77,78,79,79,80,81,82,82,83,84,84,85,86,87,88,88,89,90,91,91,92,93,94,95,95,96,97,98,99,100,100,101,102,103,104,105,106,107,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,134,135,136,137,138,140,141,142,143,144,146,147,148,150,151,152,154,155,157,158,160,161,163,164,166,168,169,171,173,175,176,178,180,182,184,186,189,191,193,196,199,201,204,207,210,214,218,222,226,232,238,246,255,
};
/* white on black */
static uint8_t ac_fg255[256] = {
0,5,9,13,16,19,22,24,27,29,32,34,36,38,40,42,44,46,48,50,52,54,56,57,59,61,62,64,65,67,69,70,72,73,75,76,77,79,80,82,83,84,86,87,88,90,91,92,93,95,96,97,98,100,101,102,103,104,105,107,108,109,110,111,112,113,114,115,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,133,134,135,136,137,138,139,140,140,141,142,143,144,145,146,147,148,149,150,150,151,152,153,154,155,156,157,157,158,159,160,161,162,163,163,164,165,166,167,167,168,169,170,171,171,172,173,174,175,175,176,177,178,178,179,180,181,181,182,183,184,184,185,186,187,187,188,189,190,190,191,192,193,193,194,195,195,196,197,198,198,199,200,200,201,202,202,203,204,205,205,206,207,207,208,209,209,210,211,211,212,213,213,214,215,215,216,216,217,218,218,219,220,220,221,222,222,223,224,224,225,225,226,227,227,228,229,229,230,230,231,232,232,233,233,234,235,235,236,236,237,238,238,239,239,240,241,241,242,242,243,244,244,245,245,246,247,247,248,248,249,249,250,251,251,252,252,253,253,254,254,255,
};

static uint8_t map_bow(uint8_t alpha) {
    return 255 - ac_fg0[alpha];
}

static uint8_t map_wob(uint8_t alpha) {
    return ac_fg255[alpha];
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
        error = FT_Outline_EmboldenXY(&face->glyph->outline, 21, 21);
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
            int pos_y = y + pen_y - face->glyph->bitmap_top;
            if (pos_y < 0 || pos_y >= height) {
                continue;
            }

            for (int x = 0; x < bitmap.width; x ++) {
                uint32_t c = bitmap.buffer[y * bitmap.pitch + x];
 
                int32_t fir[5] = { 0x0, 0x55, 0x56, 0x55, 0x0 };
                for (int dx = -2; dx <= 2; dx ++) {
                    int pos_x = dx + x + pen_x + face->glyph->bitmap_left;
                    if (pos_x < 0 || pos_x >= WIDTH * 3) {
                        continue;
                    }

                    uint32_t x = picture[pos_x + WIDTH * 3 * pos_y];
                    x += (c * fir[dx+2] + 128) >> 8;
                    if (x > 255) {
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
    for (int y = 0; y < height; y += 1) {
        row_pointers[y] = (png_byte *) malloc(png_get_rowbytes(png_ptr, info_ptr));
        for (int x = 0; x < WIDTH; x += 1) {
            row_pointers[y][x*4+0] = map_bow(picture[y * WIDTH * 3 + x*3+0]);
            row_pointers[y][x*4+1] = map_bow(picture[y * WIDTH * 3 + x*3+1]);
            row_pointers[y][x*4+2] = map_bow(picture[y * WIDTH * 3 + x*3+2]);
            row_pointers[y][x*4+3] = 0xff;
        }
    }

    png_write_image(png_ptr, row_pointers);
    png_write_end(png_ptr, NULL);
}
