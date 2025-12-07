#include <png.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc < 3) return 0;
    FILE *fp = fopen(argv[1], "rb");
    char* outfile = argv[2];
    if (!fp) return 0;

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) return 0;

    /* Provide I/O */
    FILE *fp = fopen(filename, "rb");
    png_init_io(png, fp);

    png_infop info = png_create_info_struct(png);
    if (!info) {
        png_destroy_read_struct(&png, NULL, NULL);
        return 0;
    }

    if (setjmp(png_jmpbuf(png))) {
        /* Swallow all libpng longjmp errors. */
        png_destroy_read_struct(&png, &info, NULL);
        return 0;
    }

    /// Insert APIs to test
    /// Some interesting APIs to test that modify the PNG attributes:
    /// png_set_expand, png_set_gray_to_rgb, png_set_palette_to_rgb, png_set_filler, png_set_scale_16, png_set_packing
    /// Some interesting APIs to test that fetch the PNG attributes:
    /// png_get_channels, png_get_color_type, png_get_rowbytes, png_get_image_width, png_get_image_height, 

    /// Optional write API
            FILE *out = fopen(outfile, "wb");
    if (!out) { perror("open output"); return 1; }

    png_structp wpng = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop   winfo = png_create_info_struct(wpng);
    if (!wpng || !winfo) return 1;

    if (setjmp(png_jmpbuf(wpng))) {
        fclose(out);
        png_destroy_write_struct(&wpng, &winfo);
        return 1;
    }

    png_init_io(wpng, out);

    png_set_IHDR(wpng, winfo,
                 width, height, 8,
                 PNG_COLOR_TYPE_RGBA,
                 PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_BASE,
                 PNG_FILTER_TYPE_BASE);

    png_write_info(wpng, winfo);
    png_write_image(wpng, rows);
    png_write_end(wpng, winfo);
    return 0;
}
