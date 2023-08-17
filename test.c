#include "qtcf.h"
#include "qtc3.h"
#include "qtc8b.h"

#include "utils.h"

#include "pgm.h"
#include "bps.h"
#include "xbn.h"
#include "filt_up.h"

#include "canada.h"
#include "canada_xs.h"
#include "canada_l.h"
#include "test_patterns.h"
// #include "jwst_nebula.h"

#include <stdio.h>
#include <string.h>

bool img_equal(const u8 *img1_bits, const u8 *img2_bits, u16 w, u16 h)
{
    u32 size = (w + 7) / 8 * h;

    for (u32 i = 0; i < size; i++)
    {
        if (img1_bits[i] != img2_bits[i])
        {
            return false;
        }
    }

    return true;
}

bool arr_equal(const u8 *arr1, const u8 *arr2, uint32_t size)
{
    for (u32 i = 0; i < size; i++)
    {
        if (arr1[i] != arr2[i])
        {
            return false;
        }
    }

    return true;
}

void print_img(const u8 *pix, u16 w, u16 h)
{
    const u8 *row = pix;
    u16 w_byte = (w + 7) / 8;

    for (u16 y = 0; y < h; y++)
    {
        for (u16 x = 0; x < w; x++)
        {
            if (row[x / 8] & (1 << (x % 8)))
            {
                printf("⬜");
            }
            else
            {
                printf("⬛");
            }
        }
        printf("\n");
        row += w_byte;
    }
}

void print_img_diff(const u8 *img1, const u8 *img2, u16 w, u16 h)
{

    for (u16 y = 0; y < h; y++)
    {
        for (u16 x = 0; x < w; x++)
        {
            if (*img1 == *img2)
            {
                printf("⬛");
            }
            else
            {
                printf("⬜");
            }

            img1++;
            img2++;
        }
        printf("\n");
    }
}

void print_arr_hex(FILE *stream, const u8 *arr, u32 n, u16 bpl)
{
    u32 i = 0;

    while (i < n)
    {
        u32 line_end = MIN(i + bpl, n);
        while (i < line_end)
        {
            fprintf(stream, "%02X", arr[i]);
            i++;
        }
        fprintf(stream, "\n");
    }
}

void test_qtc_img(const u8 *in, u16 w, u16 h, bool print_qtc)
{
    u32 in_size = (w + 7) / 8 * h;

    u8 *qtc;
    u32 qtc_size;

    qtc = qtcf_encode(in, w, h, &qtc_size);

    printf("size: %u, cr: %f\n", qtc_size, 1.0 * in_size / qtc_size);

    if (print_qtc)
    {
        print_arr_hex(stdout, qtc, qtc_size, 150);
    }

    u8 *out = qtcf_decode(qtc, w, h, &qtc_size);

    assert(arr_equal(in, out, in_size));

    free(qtc);
    free(out);
}

void test_qtc8b_img(const u8 *in, u16 w, u16 h, bool print_qtc)
{
    u32 in_size = w * h;

    u8 *qtc;
    u32 qtc_size;

    qtc = qtc8b_encode(in, w, h, &qtc_size);

    printf("size: %u, cr: %f\n", qtc_size, 1.0 * in_size / qtc_size);

    if (print_qtc)
    {
        print_arr_hex(stdout, qtc, qtc_size, 150);
    }

    // u8 *out = qtc2_decode(qtc, w, h, &qtc_size);

    // assert(arr_equal(in, out, in_size));

    free(qtc);
    // free(out);
}

uint8_t *gs8_qtc_encode(const uint8_t *data, uint16_t w, uint16_t h, uint32_t *out_size)
{
    uint32_t bp_size;

    uint8_t *filt = filt_up_apply(data, w, h);

    uint8_t *bit_planes = bit_plane_slice_8(filt, w, h, &bp_size);

    free(filt);

    uint8_t *gs8c = NULL;
    uint32_t gs8c_size = 0;

    for (uint32_t bp = 0; bp < 8; bp++)
    {
        uint32_t bpc_size;
        uint8_t *bpc = qtcf_encode(bit_planes + bp_size * bp, w, h, &bpc_size);

        printf("bit %u plane compressed size: %u\n", bp, bpc_size);

        // gsprint_arr_hex(stdout, bpc, bpc_size, 150);

        if (gs8c == NULL)
        {
            gs8c = malloc(bpc_size);
            gs8c_size = bpc_size;
            memcpy(gs8c, bpc, gs8c_size);
        }
        else
        {
            gs8c = realloc(gs8c, gs8c_size + bpc_size);
            memcpy(gs8c + gs8c_size, bpc, bpc_size);
            gs8c_size += bpc_size;
        }

        free(bpc);
    }

    free(bit_planes);

    *out_size = gs8c_size;

    printf("gs8 uncompressed size = %u\n", w * h);
    printf("gs8 compressed size = %u\n", *out_size);
    printf("gs8 cr = %f\n", 1.0 * w * h / *out_size);

    return gs8c;
}

uint8_t *gs8_qtc_encode_2(const uint8_t *data, uint16_t w, uint16_t h, uint32_t *out_size)
{
    uint32_t bp_size;
    uint8_t *bit_planes = bit_plane_slice_8(data, w, h, &bp_size);

    uint32_t bpc_size;
    uint8_t *bpc = qtcf_encode(bit_planes, w, (u32)h * 8, &bpc_size);

    free(bit_planes);

    *out_size = bpc_size;

    printf("gs8 uncompressed size = %u\n", w * h);
    printf("gs8 compressed size = %u\n", *out_size);
    printf("gs8 cr = %f\n", 1.0 * w * h / *out_size);

    return bpc;
}

uint8_t *gs8_qtc_decode(const uint8_t *qtc, uint16_t w, uint16_t h)
{
    uint32_t bp_size = (w + 7) / 8 * h;
    uint8_t *bit_planes = malloc(bp_size * 8);
    memset(bit_planes, 0, bp_size * 8);

    const uint8_t *bpc = qtc;

    for (uint32_t bitpos = 0; bitpos < 8; bitpos++)
    {
        uint32_t bpc_size;
        uint8_t *bp = qtcf_decode(bpc, w, h, &bpc_size);

        bpc += bpc_size;

        memcpy(bit_planes + bitpos * bp_size, bp, bp_size);
        free(bp);
    }

    uint8_t *unsliced = bit_plane_unslice_8(bit_planes, w, h);
    free(bit_planes);

    uint8_t *defilt = filt_up_remove(unsliced, w, h);

    free(unsliced);
    return defilt;
}

#define TESTIMG_WIDTH 32
#define TESTIMG_HEIGHT 32
#define TESTIMG_SIZE TESTIMG_WIDTH / 8 * TESTIMG_HEIGHT

u8 test_bits[TESTIMG_SIZE];

int main()
{
    uint16_t w, h;
    uint32_t qtc_pix_size;
    printf("Banana qtc ");
    uint8_t *pgm_pix = pgm_read("test_assets/banana.pgm", &w, &h);
    uint8_t *qtc_pix = gs8_qtc_encode(pgm_pix, w, h, &qtc_pix_size);
    // uint8_t *out_pix = gs8_qtc_decode(qtc_pix, w, h);

    // assert(arr_equal(pgm_pix, out_pix, w * h));

    free(qtc_pix);
    free(pgm_pix);
    // free(out_pix);

    printf("Lenna qtc ");
    pgm_pix = pgm_read("test_assets/lenna.pgm", &w, &h);
    qtc_pix = gs8_qtc_encode(pgm_pix, w, h, &qtc_pix_size);
    // out_pix = gs8_qtc_decode(qtc_pix, w, h);

    // assert(arr_equal(pgm_pix, out_pix, w * h));

    free(qtc_pix);
    free(pgm_pix);
    // free(out_pix);

    printf("Canada qtc ");
    pgm_pix = pgm_read("test_assets/canada.pgm", &w, &h);
    qtc_pix = gs8_qtc_encode(pgm_pix, w, h, &qtc_pix_size);
    // out_pix = gs8_qtc_decode(qtc_pix, w, h);

    // assert(arr_equal(pgm_pix, out_pix, w * h));

    free(qtc_pix);
    free(pgm_pix);
    // free(out_pix);

    printf("Esig qtc ");
    pgm_pix = pgm_read("test_assets/esig.pgm", &w, &h);
    qtc_pix = gs8_qtc_encode(pgm_pix, w, h, &qtc_pix_size);
    // out_pix = gs8_qtc_decode(qtc_pix, w, h);

    // assert(arr_equal(pgm_pix, out_pix, w * h));

    free(qtc_pix);
    free(pgm_pix);
    // free(out_pix);

    printf("Nebula qtc ");
    pgm_pix = pgm_read("test_assets/jwst_nebula.pgm", &w, &h);
    qtc_pix = gs8_qtc_encode(pgm_pix, w, h, &qtc_pix_size);
    // out_pix = gs8_qtc_decode(qtc_pix, w, h);

    // assert(arr_equal(pgm_pix, out_pix, w * h));

    free(qtc_pix);
    free(pgm_pix);
    // free(out_pix);

    printf("DF qtc ");
    pgm_pix = pgm_read("test_assets/jwst_df.pgm", &w, &h);
    qtc_pix = gs8_qtc_encode(pgm_pix, w, h, &qtc_pix_size);
    // out_pix = gs8_qtc_decode(qtc_pix, w, h);

    // assert(arr_equal(pgm_pix, out_pix, w * h));

    // u32 counts[16] = {0};

    // for (u32 i = 0; i < 2 * qtc_pix_size; i++)
    // {
    //     counts[na_read(qtc_pix, i)]++;
    // }

    // for (u8 i = 0; i < 16; i++)
    // {
    //     printf("%X: %u\n", i, counts[i]);
    // }

    free(pgm_pix);
    free(qtc_pix);
    // free(out_pix);

    printf("park qtc ");
    pgm_pix = pgm_read("test_assets/park.pgm", &w, &h);
    qtc_pix = gs8_qtc_encode(pgm_pix, w, h, &qtc_pix_size);

    free(pgm_pix);
    free(qtc_pix);

    // printf("Globe qtc ");
    // test_qtc_img(globe_bits, GLOBE_WIDTH, GLOBE_HEIGHT, false);

    // printf("Globe XXS qtc ");
    // test_qtc_img(globe_xxs_bits, GLOBE_XXS_WIDTH, GLOBE_XXS_HEIGHT, false);

    // printf("Map qtc ");
    // test_qtc_img(map_bits, MAP_WIDTH, MAP_HEIGHT, false);

    // printf("Map 320x144 qtc ");
    // test_qtc_img(map_320x144_bits, MAP_320X144_WIDTH, MAP_320X144_HEIGHT, false);

    // printf("Map XL qtc ");
    // test_qtc_img(map_xl_bits, MAP_XL_WIDTH, MAP_XL_HEIGHT, false);

    // printf("Book qtc ");
    // test_qtc_img(book_bmp_bits, BOOK_BMP_WIDTH, BOOK_BMP_HEIGHT, false);

    // printf("Horse qtc ");
    // test_qtc_img(horse_bits, HORSE_WIDTH, HORSE_HEIGHT, false);

    // printf("Grass qtc ");
    // test_qtc_img(grass_bits, GRASS_WIDTH, GRASS_HEIGHT, false);

    // printf("Canada qtc ");
    // test_qtc_img(canada_bits, CANADA_WIDTH, CANADA_HEIGHT, true);

    // printf("Canada L qtc ");
    // test_qtc_img(canada_l_bits, CANADA_L_WIDTH, CANADA_L_HEIGHT, false);

    // printf("Canada XS qtc ");
    // test_qtc_img(canada_xs_bits, CANADA_XS_WIDTH, CANADA_XS_HEIGHT, true);

    // // printf("Nebula qtc ");
    // // test_qtc_img(jwst_nebula_bits, JWST_NEBULA_WIDTH, JWST_NEBULA_HEIGHT, false);

    // FILE *f = fopen("random.bits", "rb");

    // for (uint32_t i = 0; i < TESTIMG_SIZE; i++)
    // {
    //     test_bits[i] = getc(f);
    // }

    // fclose(f);

    // printf("Random ");
    // test_qtc_img(test_bits, TESTIMG_WIDTH, TESTIMG_HEIGHT, true);

    // printf("Test 2 qtc ");
    // test_qtc_img(test2_bits, TEST2_WIDTH, TEST2_HEIGHT, true);

    // printf("3/4 checker qtc ");
    // test_qtc_img(test0_bits, TEST0_WIDTH, TEST0_HEIGHT, true);

    // printf("fill to bottom qtc ");
    // test_qtc_img(test1_bits, TEST1_WIDTH, TEST1_HEIGHT, true);

    // for (u32 i = 0; i < TESTIMG_SIZE; i++)
    // {
    //     test_bits[i] = 0xFF;
    // }

    // printf("Fill white qtc ");
    // test_qtc_img(test_bits, TESTIMG_WIDTH, TESTIMG_HEIGHT, true);

    // for (u32 i = 0; i < TESTIMG_SIZE; i++)
    // {
    //     test_bits[i] = 0x00;
    // }

    // printf("Fill black qtc ");
    // test_qtc_img(test_bits, TESTIMG_WIDTH, TESTIMG_HEIGHT, true);

    // for (u32 i = 0; i < TESTIMG_SIZE; i++)
    // {
    //     test_bits[i] = ((i / (TESTIMG_WIDTH / 8)) & 1) ? 0xAA : 0x55;
    // }

    // printf("Checker qtc ");
    // test_qtc_img(test_bits, TESTIMG_WIDTH, TESTIMG_HEIGHT, true);

    // for (u32 i = 0; i < TESTIMG_SIZE; i++)
    // {
    //     test_bits[i] = ((i / (TESTIMG_WIDTH / 8)) & 1) ? 0 : 0xFF;
    // }

    // printf("H stripes qtc ");
    // test_qtc_img(test_bits, TESTIMG_WIDTH, TESTIMG_HEIGHT, true);

    // for (u32 i = 0; i < TESTIMG_SIZE; i++)
    // {
    //     test_bits[i] = 0x55;
    // }

    // printf("V stripes qtc ");
    // test_qtc_img(test_bits, TESTIMG_WIDTH, TESTIMG_HEIGHT, false);

    return 0;
}
