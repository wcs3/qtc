#include "qtc.h"
#include "assets.h"

#include <stdio.h>

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

int main()
{
    u8 *qtc;
    u32 qtc_byte_cnt;
    qtc_encode(map_bits, MAP_WIDTH, MAP_HEIGHT, &qtc, &qtc_byte_cnt);

    printf("Uncompressed size: %u bytes\n", (MAP_WIDTH + 7) / 8 * MAP_HEIGHT);
    printf("Compressed size: %u bytes\n", qtc_byte_cnt);

    u8 *pix;

    qtc_decode(qtc, MAP_WIDTH, MAP_HEIGHT, &pix);

    // print_img(globe_bits, GLOBE_WIDTH, GLOBE_HEIGHT);

    // printf("\n");

    // print_img(pix, GLOBE_WIDTH, GLOBE_HEIGHT);

    return 0;
}
