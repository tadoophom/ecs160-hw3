//Compile: gcc -shared -Wall -O3 -fPIC custom_mutator.c -o custom_mutator.so -lz

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <zlib.h>

static const uint8_t PNG_SIG[8] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };

typedef struct {
    uint8_t *mutated_buf;
    size_t   buf_capacity;
    unsigned int seed;
} mutator_state_t;

typedef struct {
    uint32_t length;
    uint8_t  type[4];
    uint8_t *data;
    uint32_t crc;
    int      force_corrupt_crc;
} png_chunk_t;

typedef struct {
    png_chunk_t *chunks;
    size_t count;
} png_file_t;

static uint32_t read_be32(const uint8_t *ptr) {
    return ((uint32_t)ptr[0] << 24) | ((uint32_t)ptr[1] << 16) | 
           ((uint32_t)ptr[2] << 8)  | (uint32_t)ptr[3];
}

static void write_be32(uint8_t *ptr, uint32_t val) {
    ptr[0] = (val >> 24) & 0xFF;
    ptr[1] = (val >> 16) & 0xFF;
    ptr[2] = (val >> 8)  & 0xFF;
    ptr[3] = (val)       & 0xFF;
}

static uint32_t calculate_chunk_crc(const uint8_t *type, const uint8_t *data, uint32_t len) {
    uint32_t crc = crc32(0L, Z_NULL, 0);
    crc = crc32(crc, type, 4);
    if (len > 0 && data != NULL) {
        crc = crc32(crc, data, len);
    }
    return crc;
}

static int parse_png(const uint8_t *in, size_t size, png_file_t *out_png) {
    if (size < 8 || memcmp(in, PNG_SIG, 8) != 0) return 0;

    out_png->count = 0;
    out_png->chunks = NULL;
    size_t pos = 8;
    size_t capacity = 0;

    while (pos + 12 <= size) {
        if (out_png->count >= capacity) {
            capacity = (capacity == 0) ? 8 : capacity * 2;
            out_png->chunks = realloc(out_png->chunks, capacity * sizeof(png_chunk_t));
        }

        png_chunk_t *chunk = &out_png->chunks[out_png->count];
        
        chunk->length = read_be32(in + pos);
        pos += 4;

        if (pos + 4 + chunk->length + 4 > size) break;

        memcpy(chunk->type, in + pos, 4);
        pos += 4;

        chunk->data = NULL;
        if (chunk->length > 0) {
            chunk->data = malloc(chunk->length);
            memcpy(chunk->data, in + pos, chunk->length);
            pos += chunk->length;
        }

        chunk->crc = read_be32(in + pos);
        chunk->force_corrupt_crc = 0;
        pos += 4;

        out_png->count++;

        if (memcmp(chunk->type, "IEND", 4) == 0) break;
    }
    return 1;
}

static size_t rebuild_png(png_file_t *png, uint8_t *out_buf, size_t max_size) {
    size_t pos = 0;
    if (max_size < 8) return 0;

    memcpy(out_buf, PNG_SIG, 8);
    pos += 8;

    for (size_t i = 0; i < png->count; i++) {
        png_chunk_t *chunk = &png->chunks[i];
        size_t total_chunk_len = 12 + chunk->length;

        if (pos + total_chunk_len > max_size) break;

        write_be32(out_buf + pos, chunk->length);
        pos += 4;

        memcpy(out_buf + pos, chunk->type, 4);
        pos += 4;

        if (chunk->length > 0 && chunk->data) {
            memcpy(out_buf + pos, chunk->data, chunk->length);
            pos += chunk->length;
        }

        uint32_t final_crc = chunk->force_corrupt_crc ? 
                             chunk->crc : 
                             calculate_chunk_crc(chunk->type, chunk->data, chunk->length);
        write_be32(out_buf + pos, final_crc);
        pos += 4;
    }
    return pos;
}

static void free_png(png_file_t *png) {
    if (png->chunks) {
        for (size_t i = 0; i < png->count; i++) {
            free(png->chunks[i].data);
        }
        free(png->chunks);
    }
}

void mutate_ihdr(png_file_t *png) {
    for (size_t i = 0; i < png->count; i++) {
        if (memcmp(png->chunks[i].type, "IHDR", 4) == 0 && png->chunks[i].length >= 13) {
            uint8_t *d = png->chunks[i].data;
            int choice = rand() % 5;
            switch(choice) {
                case 0: write_be32(d, rand()); break;
                case 1: write_be32(d + 4, rand()); break;
                case 2: d[8] = rand() % 256; break;
                case 3: d[9] = rand() % 256; break;
                case 4: write_be32(d, 0xFFFFFFFF);
                        write_be32(d + 4, 0xFFFFFFFF);
                        break;
            }
            return;
        }
    }
}

void drop_random_chunk(png_file_t *png) {
    if (png->count <= 2) return;
    size_t idx = 1 + (rand() % (png->count - 1));
    
    free(png->chunks[idx].data);
    memmove(&png->chunks[idx], &png->chunks[idx + 1], 
            sizeof(png_chunk_t) * (png->count - 1 - idx));
    png->count--;
}

void corrupt_crc_randomly(png_file_t *png) {
    if (png->count == 0) return;
    size_t idx = rand() % png->count;
    png->chunks[idx].crc = rand();
    png->chunks[idx].force_corrupt_crc = 1;
}

void *afl_custom_init(void *afl, unsigned int seed) {
    srand(seed);
    mutator_state_t *state = malloc(sizeof(mutator_state_t));
    state->mutated_buf = NULL;
    state->buf_capacity = 0;
    state->seed = seed;
    return state;
}

size_t afl_custom_fuzz(void *data, uint8_t *buf, size_t buf_size, 
                       uint8_t **out_buf, uint8_t *add_buf, 
                       size_t add_buf_size, size_t max_size) {
    
    mutator_state_t *state = (mutator_state_t *)data;

    if (state->buf_capacity < max_size) {
        state->mutated_buf = realloc(state->mutated_buf, max_size);
        state->buf_capacity = max_size;
    }

    png_file_t png;
    if (!parse_png(buf, buf_size, &png)) {
        *out_buf = buf;
        return buf_size;
    }

    int mutation_type = rand() % 100;
    if (mutation_type < 40) {
        mutate_ihdr(&png);
    } else if (mutation_type < 70) {
        drop_random_chunk(&png);
    } else {
        corrupt_crc_randomly(&png);
    }

    size_t new_len = rebuild_png(&png, state->mutated_buf, max_size);
    free_png(&png);

    if (new_len > 0) {
        *out_buf = state->mutated_buf;
        return new_len;
    }

    *out_buf = buf;
    return buf_size;
}

void afl_custom_deinit(void *data) {
    mutator_state_t *state = (mutator_state_t *)data;
    if (state) {
        free(state->mutated_buf);
        free(state);
    }
}