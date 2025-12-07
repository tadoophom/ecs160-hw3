#include <png.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc < 3) return 1;

    FILE *fp = fopen(argv[1], "rb");
    char* outfile = argv[2];

    if (fp == NULL) return 1;

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png == NULL) {
        fclose(fp);
        return 1;
    }

    /* Provide I/O */
    png_init_io(png, fp);

    png_infop info = png_create_info_struct(png);
    if (info == NULL) {
        png_destroy_read_struct(&png, NULL, NULL);
        fclose(fp);
        return 1;
    }

    png_read_info(png, info);
    png_set_expand(png);
    png_set_gray_to_rgb(png);
    png_set_palette_to_rgb(png);
    png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
    png_set_scale_16(png);
    png_set_packing(png);
    png_read_update_info(png, info);
    int channels = png_get_channels(png, info);
    int colorType = png_get_color_type(png, info);
    png_size_t rowBytes = png_get_rowbytes(png, info);
    int width = png_get_image_width(png, info);
    int height = png_get_image_height(png, info);

    png_bytep *rows = malloc(sizeof(png_bytep) * height);
    if (rows == NULL) {
        perror("Malloc failed for rows");
        png_destroy_read_struct(&png, &info, NULL);
        fclose(fp);
        return 1;
    }
    for (int i = 0; i < height; i++) {
        rows[i] = malloc(rowBytes);
        if (rows[i] == NULL) {
            perror("Malloc failed for rows");
            for (int j = 0; j < i; j++) {
                free(rows[j]);
            }
            free(rows);
            png_destroy_read_struct(&png, &info, NULL);
            fclose(fp);
            return 1;
        }
    }

    if (setjmp(png_jmpbuf(png))) {
        for (int i = 0; i < height; i++) {
            free(rows[i]);
        }
        free(rows);
        png_destroy_read_struct(&png, &info, NULL);
        fclose(fp);
        return 1;
    }

    png_read_image(png, rows);
    png_read_end(png, info);

    png_destroy_read_struct(&png, &info, NULL);
    fclose(fp);

    FILE *out = fopen(outfile, "wb");
    if (out == NULL) {
        perror("open output");
        return 2;
    }

    png_structp wpng = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop   winfo = png_create_info_struct(wpng);
    if (wpng == NULL || winfo == NULL) {
        for (int i = 0; i < height; i++) {
            free(rows[i]);
        }
        free(rows);
        png_destroy_write_struct(&wpng, &winfo);
        fclose(out);
        return 2;
    }

    if (setjmp(png_jmpbuf(wpng))) {
        for (int i = 0; i < height; i++) {
            free(rows[i]);
        }
        free(rows);
        png_destroy_write_struct(&wpng, &winfo);
        fclose(out);
        return 2;
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

    for (int i = 0; i < height; i++) {
        free(rows[i]);
    }

    free(rows);

    png_destroy_write_struct(&wpng, &winfo);
    fclose(out);

    return 0;
}