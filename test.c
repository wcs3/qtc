#include "qtc.h"
#include "qtc2.h"
#include "assets.h"
#include "utils.h"

#include <stdio.h>

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)       \
    ((byte)&0x80 ? '1' : '0'),     \
        ((byte)&0x40 ? '1' : '0'), \
        ((byte)&0x20 ? '1' : '0'), \
        ((byte)&0x10 ? '1' : '0'), \
        ((byte)&0x08 ? '1' : '0'), \
        ((byte)&0x04 ? '1' : '0'), \
        ((byte)&0x02 ? '1' : '0'), \
        ((byte)&0x01 ? '1' : '0')

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

void print_arr_bin(const u8 *arr, u32 arr_size)
{
    for (u32 i = 0; i < arr_size; i++)
    {
        printf(BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(arr[i]));
    }

    printf("\n");
}

void print_arr_hex(FILE *stream, const u8 *arr, u32 n, u16 bpl)
{
    u32 i = 0;

    while (i < n)
    {
        u32 line_end = MIN(i + bpl, n);
        while (i < line_end)
        {
            fprintf(stream, "0x%02X, ", arr[i]);
            i++;
        }
        fprintf(stream, "\n");
    }
}

void test_qtc2_fill()
{
    u8 in[] = {
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF};

    u16 w = 32;
    u16 h = 32;

    u8 *qtc;
    u32 qtc_size;

    qtc2_encode(in, w, h, &qtc, &qtc_size);

    u8 *out;

    qtc2_decode(qtc, w, h, &out);

    print_img(out, w, h);

    printf("%u\n", qtc_size);
    assert(img_equal(in, out, w, h));
}

void test_qtc_fill()
{
    u8 in[] = {
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF};

    u16 w = 32;
    u16 h = 32;

    u8 *qtc;
    u32 qtc_size;

    qtc = qtc_encode(in, w, h, &qtc_size);

    u8 *out;

    out = qtc_decode(qtc, w, h);

    print_img(out, w, h);

    printf("%u\n", qtc_size);

    print_arr_bin(qtc, qtc_size);
    assert(img_equal(in, out, w, h));
}

void test_qtc2_img(const u8 *in, u16 w, u16 h, bool print_qtc)
{
    u32 in_size = (w + 7) / 8 * h;

    u8 *qtc;
    u32 qtc_size;

    qtc2_encode(in, w, h, &qtc, &qtc_size);

    u8 *out;
    printf("cr: %f\n", 1.0 * in_size / qtc_size);

    qtc2_decode(qtc, w, h, &out);

    if (print_qtc)
    {
        print_arr_hex(stdout, qtc, qtc_size, 30);
        print_img(out, w, h);
    }

    assert(img_equal(in, out, w, h));

    free(qtc);
    free(out);
}

void test_qtc_img(const u8 *in, u16 w, u16 h, bool print_qtc)
{
    u32 in_size = (w + 7) / 8 * h;

    u8 *qtc;
    u32 qtc_size;

    qtc = qtc_encode(in, w, h, &qtc_size);

    printf("cr: %f\n", 1.0 * in_size / qtc_size);

    if (print_qtc)
    {
        print_arr_hex(stdout, qtc, qtc_size, 30);
    }

    u8 *out = qtc_decode(qtc, w, h);

    assert(img_equal(in, out, w, h));

    free(qtc);
    free(out);
}

#define TESTIMG_WIDTH 2048
#define TESTIMG_HEIGHT 2048
#define TESTIMG_SIZE TESTIMG_WIDTH / 8 * TESTIMG_HEIGHT

u8 test_bits[TESTIMG_SIZE];



int main()
{
    printf("Globe qtc2 ");
    test_qtc2_img(globe_bits, GLOBE_WIDTH, GLOBE_HEIGHT, false);
    printf("Globe qtc ");
    test_qtc_img(globe_bits, GLOBE_WIDTH, GLOBE_HEIGHT, false);

    printf("Globe XXS qtc2 ");
    test_qtc2_img(globe_xxs_bits, GLOBE_XXS_WIDTH, GLOBE_XXS_HEIGHT, false);
    printf("Globe XXS qtc ");
    test_qtc_img(globe_xxs_bits, GLOBE_XXS_WIDTH, GLOBE_XXS_HEIGHT, false);

    printf("Map qtc2 ");
    test_qtc2_img(map_bits, MAP_WIDTH, MAP_HEIGHT, false);
    printf("Map qtc ");
    test_qtc_img(map_bits, MAP_WIDTH, MAP_HEIGHT, false);

    printf("Map 320x144 qtc2 ");
    test_qtc2_img(map_320x144_bits, MAP_320X144_WIDTH, MAP_320X144_HEIGHT, false);
    printf("Map 320x144 qtc ");
    test_qtc_img(map_320x144_bits, MAP_320X144_WIDTH, MAP_320X144_HEIGHT, false);

    printf("Map XL qtc2 ");
    test_qtc2_img(map_xl_bits, MAP_XL_WIDTH, MAP_XL_HEIGHT, false);
    printf("Map XL qtc ");
    test_qtc_img(map_xl_bits, MAP_XL_WIDTH, MAP_XL_HEIGHT, false);

    printf("Book qtc2 ");
    test_qtc2_img(book_bmp_bits, BOOK_BMP_WIDTH, BOOK_BMP_HEIGHT, false);
    printf("Book qtc ");
    test_qtc_img(book_bmp_bits, BOOK_BMP_WIDTH, BOOK_BMP_HEIGHT, false);

    printf("Horse qtc2 ");
    test_qtc2_img(horse_bits, HORSE_WIDTH, HORSE_HEIGHT, false);
    printf("Horse qtc ");
    test_qtc_img(horse_bits, HORSE_WIDTH, HORSE_HEIGHT, false);

    printf("Grass qtc2 ");
    test_qtc2_img(grass_bits, GRASS_WIDTH, GRASS_HEIGHT, false);
    printf("Grass qtc ");
    test_qtc_img(grass_bits, GRASS_WIDTH, GRASS_HEIGHT, false);

    printf("Canada qtc2 ");
    test_qtc2_img(canada_bits, CANADA_WIDTH, CANADA_HEIGHT, false);
    printf("Canada qtc ");
    test_qtc_img(canada_bits, CANADA_WIDTH, CANADA_HEIGHT, false);

    printf("Canada XL qtc2 ");
    test_qtc2_img(canada_xl_bits, CANADA_XL_WIDTH, CANADA_XL_HEIGHT, false);
    printf("Canada XL qtc ");
    test_qtc_img(canada_xl_bits, CANADA_XL_WIDTH, CANADA_XL_HEIGHT, false);

    FILE *rand_file = fopen("random.bits", "rb");

    for (u32 i = 0; i < TESTIMG_SIZE; i++)
    {
        test_bits[i] = fgetc(rand_file);
    }

    printf("Rand qtc2 ");
    test_qtc2_img(test_bits, TESTIMG_WIDTH, TESTIMG_HEIGHT, false);
    printf("Rand qtc ");
    test_qtc_img(test_bits, TESTIMG_WIDTH, TESTIMG_HEIGHT, false);

    for (u32 i = 0; i < TESTIMG_SIZE; i++)
    {
        test_bits[i] = 0xFF;
    }

    printf("Fill white qtc2 ");
    test_qtc2_img(test_bits, TESTIMG_WIDTH, TESTIMG_HEIGHT, false);
    printf("Fill white qtc ");
    test_qtc_img(test_bits, TESTIMG_WIDTH, TESTIMG_HEIGHT, false);

    for (u32 i = 0; i < TESTIMG_SIZE; i++)
    {
        test_bits[i] = 0x00;
    }

    printf("Fill black qtc2 ");
    test_qtc2_img(test_bits, TESTIMG_WIDTH, TESTIMG_HEIGHT, false);
    printf("Fill black qtc ");
    test_qtc_img(test_bits, TESTIMG_WIDTH, TESTIMG_HEIGHT, false);

    for (u32 i = 0; i < TESTIMG_SIZE; i++)
    {
        test_bits[i] = ((i / (TESTIMG_WIDTH / 8)) & 1) ? 0xAA : 0x55;
    }

    printf("Checker qtc2 ");
    test_qtc2_img(test_bits, TESTIMG_WIDTH, TESTIMG_HEIGHT, false);
    printf("Checker qtc ");
    test_qtc_img(test_bits, TESTIMG_WIDTH, TESTIMG_HEIGHT, false);

    for (u32 i = 0; i < TESTIMG_SIZE; i++)
    {
        test_bits[i] = ((i / (TESTIMG_WIDTH / 8)) & 1) ? 0 : 0xFF;
    }

    printf("H stripes qtc2 ");
    test_qtc2_img(test_bits, TESTIMG_WIDTH, TESTIMG_HEIGHT, false);
    printf("H stripes qtc ");
    test_qtc_img(test_bits, TESTIMG_WIDTH, TESTIMG_HEIGHT, false);

    for (u32 i = 0; i < TESTIMG_SIZE; i++)
    {
        test_bits[i] = 0x55;
    }

    printf("V stripes qtc2 ");
    test_qtc2_img(test_bits, TESTIMG_WIDTH, TESTIMG_HEIGHT, false);
    printf("V stripes qtc ");
    test_qtc_img(test_bits, TESTIMG_WIDTH, TESTIMG_HEIGHT, false);

    return 0;
}
