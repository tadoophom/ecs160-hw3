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
  (void)argc;
  (void)argv;
  return 0;
}
