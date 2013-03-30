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

void draw_glyph(FT_GlyphSlot glyph, int dx, int dy) {
    FT_Glyph copy;
    FT_Error error;

    error = FT_Get_Glyph(glyph, &copy);
    FT_Vector position = { dx, dy };
    error = FT_Glyph_To_Bitmap(&copy, FT_RENDER_MODE_NORMAL, &position, 1);
    if (error) {
        fprintf(stderr, "FT glyph to bitmap: error %d\n", error);
        return;
    }
    FT_BitmapGlyph bitmapcopy = (FT_BitmapGlyph) copy;

    int score = score_glyph_bitmap(bitmapcopy->bitmap);
    fprintf(stdout, "Translation: (%d %d) px/64; Baseline: (%d %d) px; Size (%d %d) px; Score: %d\n", dx, dy, bitmapcopy->left, bitmapcopy->top, bitmapcopy->bitmap.width, bitmapcopy->bitmap.rows, score);
    draw_glyph_bitmap(bitmapcopy->bitmap);
    FT_Done_Glyph(copy);
    fprintf(stdout, "\n");
}

int main(int argc, char **argv) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s font_name size_in_px\n", argv[0]);
    return 1;
  }

  char *font_name = argv[1];
  int size_in_px = atoi(argv[2]);
  char* letters = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
  int letters_len = strlen(letters);

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

  /* Perform a 2D scan of the surface looking for optimal positioning in
   * 5 * 9 = 45 iterations */
  int step = 16;
  int dx = 0;
  int dy = 0;
  while (step != 0) {
    int bestscore = -0x7ffffff;
    int besti = 0;
    for (int i = 0; i < 9; i ++) {
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

      int score = 0;
      FT_Vector position = { dx + adjx, dy + adjy };
      FT_Set_Transform(face, 0, &position);
      for (int idx = 0; idx < letters_len; idx ++) {
	int glyph_index = FT_Get_Char_Index(face, letters[idx]);
	if (!glyph_index) {
	  fprintf(stderr, "FT get char index: no glyph found for codepoint %d\n", letters[idx]);
	  return 1;
	}

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

      if (score > bestscore) {
        bestscore = score;
        besti = i;
      }
    }

    dx += ((besti % 3) - 1) * step;
    dy += ((besti / 3) - 1) * step;
    step >>= 1;
  }

  FT_Set_Transform(face, NULL, NULL);
  for (int idx = 0; idx < letters_len; idx ++) {
    int glyph_index = FT_Get_Char_Index(face, letters[idx]);
    if (!glyph_index) {
      fprintf(stderr, "FT get char index: no glyph found for codepoint %d\n", letters[idx]);
      return 1;
    }

    error = FT_Load_Glyph(face, glyph_index, FT_LOAD_NO_AUTOHINT | FT_LOAD_NO_HINTING | FT_LOAD_NO_BITMAP);
    if (error) {
      fprintf(stderr, "FT load glyph: error %d\n", error);
      return 1;
    }

    draw_glyph(face->glyph, 0, 0);
    draw_glyph(face->glyph, dx, dy);
  }
}
