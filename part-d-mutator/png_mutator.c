/**
 * Custom PNG Mutator for AFL++
 * ECS160 HW3 - Part D
 *
 * A structure-aware mutator that understands PNG file format
 * to generate more effective test cases for LibPNG fuzzing.
 *
 * Mutation Strategies:
 * 1. Mutate IHDR values (width, height, bit depth, color type)
 * 2. Corrupt CRC checksums
 * 3. Mutate chunk data randomly
 * 4. Delete chunks
 * 5. Duplicate chunks
 * 6. Insert malformed chunks
 * 7. Truncate IDAT data
 *
 * Compile with:
 *   gcc -shared -Wall -O3 -fPIC png_mutator.c -o png_mutator.so -lz
 *
 * Use with AFL++:
 *   AFL_CUSTOM_MUTATOR_LIBRARY=./png_mutator.so afl-fuzz -i seeds -o out -- ./harness @@ /dev/null
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <zlib.h>

typedef struct {
    uint8_t  *buf;
    size_t   buf_size;
    unsigned int seed;
} mutator_state_t;

static const uint8_t PNG_SIGNATURE[8] = { 0x89,0x50,0x4E,0x47,0x0D,0x0A,0x1A,0x0A };

typedef struct {
    uint32_t length;
    uint8_t  type[4];
    uint8_t *data;
    uint32_t crc;
    int corrupt_crc;
} png_chunk_t;

typedef struct {
    png_chunk_t *chunks;
    size_t   num_chunks;
    size_t   capacity;
} png_data_t;

static uint32_t read_be32(const uint8_t *b) {
    return ((uint32_t)b[0] << 24)| ((uint32_t)b[1] << 16)| ((uint32_t)b[2] <<  8)| (uint32_t)b[3];
}

static void write_be32(uint8_t *b , uint32_t v){
    b[0] =(v>>24) & 0xFF;
    b[1] =(v>>16) & 0xFF;
    b[2] =(v>> 8) & 0xFF;
    b[3] =v & 0xFF;
}

static uint32_t calculate_crc(const uint8_t *t , const uint8_t *d , uint32_t len ){
    uint32_t c = crc32(0L , Z_NULL , 0);
    c = crc32(c , t , 4);
    if (d && len > 0)   c = crc32(c , d , len);
    return c;
}

static uint32_t rand_range (uint32_t m) {
    if (m == 0) return 0;
    return rand() % m;
}

static void png_data_init(png_data_t *p){
    p->chunks = NULL;
    p->num_chunks = 0;
    p->capacity   = 0;
}

static void png_data_free(png_data_t *p){
    if (p->chunks){
        for(size_t i=0; i<p->num_chunks; ++i){
            free( p->chunks[i].data );
        }
        free(p->chunks);
    }
    p->chunks = NULL;
    p->num_chunks = 0;
    p->capacity = 0;
}

static int png_data_add_chunk(png_data_t *p , png_chunk_t *c){
    if (p->num_chunks >= p->capacity){
        size_t newcap = (p->capacity == 0 ? 16 : p->capacity * 2);
        png_chunk_t *nc = realloc(p->chunks , newcap * sizeof(png_chunk_t));
        if (!nc) return -1;
        p->chunks = nc;
        p->capacity = newcap;
    }
    p->chunks[p->num_chunks++] = *c ;
    return 0;
}

static int parse_png(const uint8_t *buf , size_t size , png_data_t *png){
    png_data_init(png);

    if (size < 8 || memcmp(buf , PNG_SIGNATURE , 8) != 0)
        return -1;

    size_t pos = 8;

    while (pos + 12 <= size){
        png_chunk_t c;
        memset(&c , 0 , sizeof(c));

        c.length = read_be32(buf + pos); pos += 4;

        if (c.length > 0x7FFFFFFF || pos + 4 + c.length + 4 > size)
            break;

        memcpy(c.type , buf + pos , 4); pos += 4;

        if (c.length > 0){
            c.data = malloc(c.length);
            if (!c.data){ png_data_free(png); return -1; }
            memcpy(c.data , buf + pos , c.length);
        } else c.data = NULL;

        pos += c.length;

        c.crc = read_be32(buf + pos);  pos += 4;
        c.corrupt_crc = 0;

        if (png_data_add_chunk(png , &c) < 0){
            free(c.data);
            png_data_free(png);
            return -1;
        }

        if (memcmp(c.type , "IEND" , 4) == 0)
            break;
    }

    return (png->num_chunks > 0) ? 0 : -1;
}

static size_t rebuild_png(png_data_t *p , uint8_t *out , size_t maxsz){
    size_t pos = 0;

    if (pos + 8 > maxsz) return 0;
    memcpy(out , PNG_SIGNATURE , 8);
    pos += 8;

    for (size_t i = 0; i < p->num_chunks; ++i){
        png_chunk_t *c = &p->chunks[i];
        size_t required = 4 + 4 + c->length + 4;

        if (pos + required > maxsz) break;

        write_be32(out + pos , c->length); pos += 4;

        memcpy(out + pos , c->type , 4); pos += 4;

        if (c->length > 0 && c->data){
            memcpy(out + pos , c->data , c->length);
        }
        pos += c->length;

        uint32_t crc = (c->corrupt_crc ? c->crc: calculate_crc(c->type , c->data , c->length));
        write_be32(out + pos , crc); pos += 4;
    }

    return pos;
}

static void mutate_ihdr(png_data_t *png){
    for (size_t i=0; i < png->num_chunks; ++i){
        png_chunk_t *c = &png->chunks[i];

        if (!memcmp(c->type , "IHDR" , 4) &&c->length >= 13 &&c->data){
            uint8_t *d = c->data;
            int pick = rand_range(8);

            switch(pick){
                case 0: {
                    uint32_t vals[]={0,1,0xFFFFFFFF,0x7FFFFFFF,0x80000000, rand()};
                    write_be32(d , vals[rand_range(6)]);
                } break;

                case 1: {
                    uint32_t vals[]={0,1,0xFFFFFFFF,0x7FFFFFFF,0x80000000, rand()};
                    write_be32(d+4 , vals[rand_range(6)]);
                } break;

                case 2:{
                    uint8_t v[] = {0,1,2,3,4,5,7,8,9,15,16,17,32,64,255};
                    d[8] = v[rand_range(15)];
                } break;

                case 3:{
                    uint8_t v[] = {0,1,2,3,4,5,6,7,8,255};
                    d[9] = v[rand_range(10)];
                } break;

                case 4: d[10] = rand_range(256); break;

                case 5: d[11] = rand_range(256); break;

                case 6: {
                    uint8_t v[]={ 0,1,2,3,255 };
                    d[12] = v[rand_range(5)];
                } break;

                case 7:
                    write_be32(d,0xFFFFFFFF);
                    write_be32(d + 4, 0xFFFFFFFF);
                    d[8]= 255;
                    d[9]= 255;
                    break;
            }
            break;
        }
    }
}

static void mutate_chunk_data(png_data_t *p){
    if (p->num_chunks == 0) return;

    size_t idat = SIZE_MAX ;
    size_t any  = SIZE_MAX ;

    for (size_t i=0; i<p->num_chunks; ++i){
        png_chunk_t *c=&p->chunks[i];
        if (c->length > 0 && c->data){
            any = i;
            if (!memcmp(c->type , "IDAT" , 4)) idat = i;
        }
    }

    size_t idx = (idat != SIZE_MAX && rand_range(100) < 60)? idat : any;

    if (idx == SIZE_MAX) return;

    png_chunk_t *c = &p->chunks[idx];
    uint32_t times = 1 + rand_range(c->length/10 + 1);

    for(uint32_t m=0; m<times; ++m){
        uint32_t pos = rand_range(c->length);
        int kind = rand_range(7);

        switch(kind){
            case 0: c->data[pos] ^= (1 << rand_range(8)); break;
            case 1: c->data[pos] = rand_range(256); break;
            case 2: c->data[pos]++; break;
            case 3: c->data[pos]--; break;
            case 4: c->data[pos] = 0; break;
            case 5: c->data[pos] = 0xFF; break;
            case 6:
                if (c->length > 1){
                    uint32_t p2 = rand_range(c->length);
                    uint8_t tmp = c->data[pos];
                    c->data[pos] = c->data[p2];
                    c->data[p2] = tmp;
                }
                break;
        }
    }
}

static void corrupt_crc(png_data_t *p){
    if (p->num_chunks == 0) return;

    size_t idx = rand_range(p->num_chunks);
    int r = rand_range(5);

    switch(r){
        case 0:p->chunks[idx].crc= 0; break;
        case 1:p->chunks[idx].crc= 0xFFFFFFFF; break;
        case 2:p->chunks[idx].crc^= (1u << rand_range(32)); break;
        case 3:p->chunks[idx].crc= rand(); break;
        case 4:p->chunks[idx].crc++; break;
    }

    p->chunks[idx].corrupt_crc = 1;
}

static void delete_chunk(png_data_t *p){
    if (p->num_chunks < 3) return;

    size_t idx = 1 + rand_range(p->num_chunks - 1);

    if (!memcmp(p->chunks[idx].type , "IEND" , 4) &&
         rand_range(100)< 90){
        if (p->num_chunks> 2)
            idx = 1 + rand_range(p->num_chunks - 2);
        else return;
    }

    free(p->chunks[idx].data);

    memmove(&p->chunks[idx] , &p->chunks[idx+1],(p->num_chunks - idx - 1)*sizeof(png_chunk_t));

    p->num_chunks--;
}

static void duplicate_chunk(png_data_t *p){
    if (p->num_chunks == 0) return;

    size_t idx = rand_range(p->num_chunks);
    png_chunk_t *src = &p->chunks[idx];

    png_chunk_t dup;
    dup.length = src->length;
    memcpy(dup.type , src->type , 4);
    dup.crc = src->crc;
    dup.corrupt_crc = src->corrupt_crc;

    if (src->length > 0 && src->data){
        dup.data = malloc(src->length);
        if (!dup.data) return;
        memcpy(dup.data , src->data , src->length);
    } else dup.data= NULL;

    if (p->num_chunks >= p->capacity){
        size_t newcap= (p->capacity ? p->capacity * 2 : 16);
        png_chunk_t *nc = realloc(p->chunks , newcap * sizeof(png_chunk_t));
        if (!nc){ free(dup.data); return; }
        p->chunks= nc;
        p->capacity= newcap;
    }

    memmove(&p->chunks[idx+2] , &p->chunks[idx+1],(p->num_chunks - idx - 1)*sizeof(png_chunk_t));

    p->chunks[idx+1] = dup;
    p->num_chunks++;
}

static void insert_random_chunk(png_data_t *p){
    if (p->num_chunks < 2) return;

    png_chunk_t c;
    memset(&c,0, sizeof(c));

    int mode = rand_range(4);

    switch(mode){
        case 0:{
            const char *tlist[]={"tEXt","zTXt","gAMA","cHRM","sRGB","pHYs","tIME"};
            memcpy(c.type , tlist[rand_range(7)] , 4);
        } break;
        case 1:
            for(int i=0;i<4;i++) c.type[i] = 'A' + rand_range(26);
            break;
        case 2:
            for(int i=0;i<4;i++) c.type[i] = 'a' + rand_range(26);
            break;
        case 3:
            for(int i=0;i<4;i++) c.type[i] = rand_range(256);
            break;
    }

    uint32_t lens[] = {0,1,10,100, rand_range(500)};
    c.length = lens[rand_range(5)];

    if (c.length > 0){
        c.data = malloc(c.length);
        if (!c.data) return;
        for(uint32_t i=0;i<c.length;i++) c.data[i] = rand_range(256);
    } else c.data=NULL;

    c.crc = calculate_crc(c.type , c.data , c.length);
    c.corrupt_crc = 0;

    size_t pos = 1 + rand_range(p->num_chunks - 1);

    if (p->num_chunks >= p->capacity){
        size_t newcap = (p->capacity ? p->capacity * 2 : 16);
        png_chunk_t *nc = realloc(p->chunks , newcap * sizeof(png_chunk_t));
        if (!nc){ free(c.data); return; }
        p->chunks = nc;
        p->capacity = newcap;
    }

    memmove(&p->chunks[pos+1] , &p->chunks[pos],(p->num_chunks - pos)*sizeof(png_chunk_t));

    p->chunks[pos] = c;
    p->num_chunks++;
}

static void truncate_idat(png_data_t *p){
    for(size_t i=0;i<p->num_chunks;++i){
        png_chunk_t *c = &p->chunks[i];
        if (!memcmp(c->type,"IDAT",4) && c->length > 1 && c->data){
            c->length = 1 + rand_range(c->length - 1);
            break;
        }
    }
}

static size_t mutate_random(uint8_t *b , size_t size , size_t maxsz){
    if (size == 0){
        size= 1 + rand_range(100);
        if (size > maxsz) size = maxsz;
        for(size_t i=0;i<size;i++) b[i] = rand_range(256);
        return size;
    }

    uint32_t n = 1 + rand_range(size/20 + 1);
    for( uint32_t i=0;i<n;i++){
        size_t pos= rand_range(size);
        b[pos]= rand_range(256);
    }
    return size;
}

void *afl_custom_init(void *afl , unsigned int seed){
    (void)afl;

    mutator_state_t *s = malloc(sizeof(mutator_state_t));
    if (!s) return NULL;

    s->seed= seed;
    s->buf= NULL;
    s->buf_size = 0;

    srand(seed);

    return s;
}

size_t afl_custom_fuzz(void *data , uint8_t *buf , size_t buf_size ,uint8_t **out_buf , uint8_t *add_buf ,size_t add_buf_size , size_t max_size){
    (void)add_buf;
    (void)add_buf_size;

    mutator_state_t *s = (mutator_state_t*)data;

    if (s->buf_size < max_size){
        uint8_t *nb = realloc(s->buf , max_size);
        if (!nb){
            *out_buf = buf;
            return buf_size;
        }
        s->buf = nb;
        s->buf_size = max_size;
    }

    memcpy(s->buf,buf, buf_size);

    png_data_t png;
    if (parse_png(buf,buf_size , &png) < 0){
        size_t ns = mutate_random(s->buf, buf_size , max_size);
        *out_buf = s->buf;
        return ns;
    }

    int r = rand_range(100);

    if(r < 20) mutate_ihdr(&png);
    else if(r< 45) mutate_chunk_data(&png);
    else if(r< 60) corrupt_crc(&png);
    else if(r< 70) delete_chunk(&png);
    else if(r< 80) duplicate_chunk(&png);
    else if(r< 90) insert_random_chunk(&png);
    else if(r< 95) truncate_idat(&png);
    else {
        mutate_ihdr(&png);
        mutate_chunk_data(&png);
    }

    size_t ns= rebuild_png(&png , s->buf , max_size);

    png_data_free(&png);

    if (ns == 0){
        memcpy(s->buf , buf , buf_size);
        ns = mutate_random(s->buf , buf_size , max_size);
    }

    *out_buf = s->buf;
    return ns;
}

void afl_custom_deinit(void *d){
    mutator_state_t *s = (mutator_state_t*)d;
    if (s){
        free(s->buf);
        free(s);
    }
}

const char *afl_custom_describe(void *d , size_t m){
    (void)d; 
    (void)m;
    return "Custom PNG structure-aware mutator for LibPNG fuzzing";
}
