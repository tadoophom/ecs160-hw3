#include <png.h>
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
  (void)filepath;

  return 0;
}
