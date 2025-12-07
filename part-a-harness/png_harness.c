#include <png.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc < 3) return 1;

    const char *inpath = argv[1];
    const char *outpath = argv[2];

    FILE *fp = fopen(inpath, "rb");
    if (fp == NULL) return 1;

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png == NULL) {
        fclose(fp);
        return 1;
    }

    png_infop info = png_create_info_struct(png);
    if (info == NULL) {
        png_destroy_read_struct(&png, NULL, NULL);
        fclose(fp);
        return 1;
    }

    if (setjmp(png_jmpbuf(png))) {
        png_destroy_read_struct(&png, &info, NULL);
        fclose(fp);
        return 1;
    }

    unsigned char sig[8];
    if (fread(sig, 1, 8, fp) != 8) {
        png_destroy_read_struct(&png, &info, NULL);
        fclose(fp);
        return 1;
    }
    if (png_sig_cmp(sig, 0, 8)) {
        png_destroy_read_struct(&png, &info, NULL);
        fclose(fp);
        return 1;
    }

    png_init_io(png, fp);
    png_set_sig_bytes(png, 8);

    png_read_info(png, info);

    png_set_expand(png);
    png_set_gray_to_rgb(png);
    png_set_palette_to_rgb(png);
    png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
    png_set_scale_16(png);
    png_set_packing(png);

    png_uint_32 width, height;
    int bit_depth, color_type, interlace_method, compression_method, filter_method;
    if (!png_get_IHDR(png, info, &width, &height, &bit_depth, &color_type,
                      &interlace_method, &compression_method, &filter_method)) {
        png_destroy_read_struct(&png, &info, NULL);
        fclose(fp);
        return 1;
    }

    png_read_update_info(png, info);

    png_size_t rowbytes = png_get_rowbytes(png, info);
    if (rowbytes == 0 || height == 0) {
        png_destroy_read_struct(&png, &info, NULL);
        fclose(fp);
        return 1;
    }

    png_bytep *rows = (png_bytep *)malloc(sizeof(png_bytep) * height);
    if (rows == NULL) {
        png_destroy_read_struct(&png, &info, NULL);
        fclose(fp);
        return 1;
    }

    for (png_uint_32 y = 0; y < height; ++y) {
        rows[y] = (png_bytep)malloc(rowbytes);
        if (rows[y] == NULL) {
            for (png_uint_32 i = 0; i < y; ++i) free(rows[i]);
            free(rows);
            png_destroy_read_struct(&png, &info, NULL);
            fclose(fp);
            return 1;
        }
    }

    png_read_image(png, rows);
    png_read_end(png, info);
    fclose(fp);

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
