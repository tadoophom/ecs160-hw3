#include <png.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>

/* 
   Requirements:
   - Accept a single filepath via argv.
   - Open the file in binary mode; bail safely on errors.
   - Initialize png_structp/png_infop, set read callbacks, and drive parsing
     enough to exercise core libpng APIs.
   - Clean up all allocations/structures on every exit path. 
   */

int main(int argc, char **argv) {
  if (argc != 2) return 0;

  const char *filepath = argv[1];
  FILE *fp = fopen(filepath, "rb");
  if (fp == NULL) return 0;

  png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (png == NULL) {
    fclose(fp);
    return 0;
  }

  png_infop info = png_create_info_struct(png);
  if (info == NULL) {
    png_destroy_read_struct(&png, NULL, NULL);
    fclose(fp);
    return 0;
  }

  if (setjmp(png_jmpbuf(png))) {
    png_destroy_read_struct(&png, &info, NULL);
    fclose(fp);
    return 0;
  }

  unsigned char header[8];
  if (fread(header, 1, 8, fp) != 8) {
    png_destroy_read_struct(&png, &info, NULL);
    fclose(fp);
    return 0;
  }

  if (png_sig_cmp(header, 0, 8)) {
    png_destroy_read_struct(&png, &info, NULL);
    fclose(fp);
    return 0;
  }

  png_set_sig_bytes(png, 8);
  png_init_io(png, fp);

  png_destroy_read_struct(&png, &info, NULL);
  fclose(fp);

  return 0;
}
