#include <ft2build.h>
#include <freetype/freetype.h>
#include <freetype/ftglyph.h>
#include <stdio.h>

void print_glyph(FT_Bitmap bitmap) {
    for (int y = 0; y < bitmap.rows; y ++) {
        for (int x = 0; x < bitmap.width; x ++) {
            unsigned char c = bitmap.buffer[y * bitmap.pitch + x];
            fprintf(stdout, "%02x ", c);
        }
        fprintf(stdout, "\n");
    }
}

int estimate_contrast(FT_Bitmap bitmap) {
    int score = 0;
    for (int y = 0; y < bitmap.rows; y ++) {
        for (int x = 0; x < bitmap.width; x ++) {
            unsigned char c = bitmap.buffer[y * bitmap.pitch + x];
            unsigned char xm = x != 0 ? bitmap.buffer[y * bitmap.pitch + x - 1] : 0;
            unsigned char xp = x != bitmap.width - 1 ? bitmap.buffer[y * bitmap.pitch + x + 1] : 0;
            unsigned char ym = y != 0 ? bitmap.buffer[(y - 1) * bitmap.pitch + x] : 0;
            unsigned char yp = y != bitmap.rows - 1 ? bitmap.buffer[(y + 1) * bitmap.pitch + x] : 0;
	    int point = xm + xp + ym + yp - 4 * c;
            score += point * point;
        }
    }
    return score;
}

int main(int argc, char **argv) {
  if (argc != 4) {
    fprintf(stderr, "Usage: %s font_name unicode_codepoint size_in_px\n", argv[0]);
    return 1;
  }

  char *font_name = argv[1];
  int unicode_codepoint = atoi(argv[2]);
  int size_in_px = atoi(argv[3]);

  FT_Library library;
  int error;

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

  int glyph_index = FT_Get_Char_Index(face, unicode_codepoint);
  if (!glyph_index) {
    fprintf(stderr, "FT get char index: no glyph found for codepoint %d\n", unicode_codepoint);
    return 1;
  }

  error = FT_Load_Glyph(face, glyph_index, FT_LOAD_NO_AUTOHINT | FT_LOAD_NO_HINTING | FT_LOAD_NO_BITMAP);
  if (error) {
    fprintf(stderr, "FT load glyph: error %d\n", error);
    return 1;
  }

  /* Perform a 2D scan of the surface looking for optimal positioning in
   * 5 * 9 = 45 iterations */
  int step = 16;
  int dx = 0;
  int dy = 0;
  while (step != 0) {
    int bestscore = 0;
    int besti = 0;
    for (int i = 0; i < 9; i ++) {
      FT_Glyph copy;

      error = FT_Get_Glyph(face->glyph, &copy);

      /* Generate adjustment coordinates:
       *
       * 0 1 2
       * 3 4 5
       * 6 7 8
       *
       * In theory we could cache 4, because last iteration's 4 score
       * could be kept in bestscore.
       */
      int adjx = ((i % 3) - 1) * step;
      int adjy = ((i / 3) - 1) * step;

      FT_Vector position = { dx + adjx, dy + adjy };
      error = FT_Glyph_To_Bitmap(&copy, FT_RENDER_MODE_NORMAL, &position, 1);
      if (error) {
        fprintf(stderr, "FT render glyph: error %d\n", error);
        return 1;
      }

      int score = estimate_contrast(((FT_BitmapGlyph) copy)->bitmap);
      if (score > bestscore) {
  	fprintf(stdout, "translated: (%d %d)/64 px, score: %d\n", (int) position.x, (int) position.y, score);
  	print_glyph(((FT_BitmapGlyph) copy)->bitmap);
        bestscore = score;
        besti = i;
      }

      FT_Done_Glyph(copy);
    }

    dx += ((besti % 3) - 1) * step;
    dy += ((besti / 3) - 1) * step;
    step >>= 1;
  }
}
