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

/* The analysis is only be carried out on (almost) vertical and (almost) horizontal lines.
 * Curves would be assumed to be too "curvy" and to always antialias, and therefore can be ignored.
 * The remaining lines denote the relevant edges of solid regions, and such regions are separated
 * by moveto instructions.
 *
 * 1. A line that's almost vertical or horizontal (= less than 0.5 px difference in either x or y
 *    endpoint coords) would be reduced to its midpoint to estimate location of its edge, and
 *    its length would be stored to be used as a weighting factor later. Data is
 *    (direction, 1-dimensional position, length). Line must be at least 1 px long to qualify.
 * 
 * 2. Once all vertical and horizontal edges have be collected, then the optimizer would
 *    split the edges into two separate lists based on direction for the optimal offset analysis.
 *
 * 3. The optimal offset will be most easily done by only storing the bottom 6 bits of the line
 *    midpoint coordinates and then averaging the lines by their weight, and then returning that value
 *    as the negative offset. This also gives upper limit for the datastructures involved:
 *
 * horiz lists: 64 elements of int32 type
 * vert lists: 64 elements of int32 type
 */
typedef struct {
    FT_Pos horiz[64];
    FT_Pos vert[64];
    FT_Pos x, y;
} optimize_state_t;

static int32_t optimize_move_to(const FT_Vector *to, void *data) {
    optimize_state_t *state = data;
    state->x = to->x;
    state->y = to->y;
    return 0;
}

static int32_t optimize_line_to(const FT_Vector *to, void *data) {
    optimize_state_t *state = data;
    FT_Pos dx = to->x - state->x;
    FT_Pos dy = to->y - state->y;
    FT_Pos len = 0.5f + sqrtf(dx * dx + dy * dy);

    if ((abs(dx) <= 32 || abs(dy) <= 32) && len >= 64) {
        //fprintf(stderr, "Found a line from (%f, %f) to (%f, %f)\n", state->x / 64.0f, state->y / 64.0f, to->x / 64.0f, to->y / 64.0f);
        FT_Pos cx = (to->x + state->x + 1) >> 1;
        FT_Pos cy = (to->y + state->y + 1) >> 1;
        int32_t is_horiz = abs(dx) < abs(dy);
        FT_Pos* list = is_horiz ? state->horiz : state->vert;
        FT_Pos idx = is_horiz ? cx : cy;
        list[idx & 0x3f] += len;
    }

    state->x = to->x;
    state->y = to->y;
    return 0;
}

static int32_t optimize_conic_to(const FT_Vector *ctrl, const FT_Vector *to, void *data) {
    optimize_state_t *state = data;
    state->x = to->x;
    state->y = to->y;
    return 0;
}

static int32_t optimize_cubic_to(const FT_Vector *ctrl1, const FT_Vector *ctrl2, const FT_Vector *to, void *data) {
    optimize_state_t *state = data;
    state->x = to->x;
    state->y = to->y;
    return 0;
}

static FT_Pos optimize_middle(FT_Pos *list) {
    FT_Pos sum = 0;
    FT_Pos count = 0;
    for (int32_t i = 0; i < 64; i += 1) {
        sum += i * list[i];
        count += list[i];
    }
    FT_Pos value = count != 0 ? sum / count : 0;
    if (value > 31) {
        value -= 64;
    }
    return value;
}

static void optimize_placement(FT_Face face, FT_Vector *pos) {
    optimize_state_t state = {};
    FT_Outline_Funcs funcs = {
        .move_to = optimize_move_to,
        .line_to = optimize_line_to,
        .conic_to = optimize_conic_to,
        .cubic_to = optimize_cubic_to,
        .shift = 0,
        .delta = 0
    };
    for (int32_t glyph_index = 1; glyph_index < face->num_glyphs; glyph_index ++) {
        build_glyph(face, glyph_index);
        FT_Outline_Decompose(&face->glyph->outline, &funcs, &state);
    }

    /*for (int32_t i = 0; i < 64; i += 1) {
        fprintf(stderr, "%02d %8d %8d\n", i, state.horiz[i], state.vert[i]);
    }*/

    pos->x = -optimize_middle(state.horiz);
    pos->y = -optimize_middle(state.vert);
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

    /* FIXME:
     *
     * we could also add a pass that attempts to scale the glyph by
     * 0.5 px and then retry the alignment, and use scaled variant if the
     * score is better.
     *
     * The offsets are not strictly speaking separable, but a full
     * 64x64 scan is very slow and almost always gives the same numbers.
     */
    FT_Vector position;
    optimize_placement(face, &position);
    fprintf(stderr, "Translating font face by (%ld, %ld) 1/64th pixels\n", position.x, position.y);
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
