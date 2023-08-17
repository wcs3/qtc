#include "bps.h"

#include <string.h>
#include <stdlib.h>

uint8_t *bit_plane_slice_8(const uint8_t *data, uint16_t w, uint16_t h, uint32_t *bp_size)
{
    uint16_t bp_row_size = (w + 7) / 8;
    *bp_size = bp_row_size * h;
    uint32_t bps_size = *bp_size * 8;

    uint8_t *bps = malloc(bps_size);
    memset(bps, 0, bps_size);

    uint8_t *bp_row = bps;

    for (uint8_t bp_i = 0; bp_i < 8; bp_i++)
    {
        const uint8_t *data_p = data;
        for (uint16_t y = 0; y < h; y++)
        {
            for (uint16_t x = 0; x < w; x++)
            {
                bp_row[x / 8] |= ((*data_p >> bp_i) & 0x1) << (x % 8);
                data_p++;
            }
            bp_row += bp_row_size;
        }
    }

    bp_row = bps;
    uint8_t *bp_row_next = bp_row + (*bp_size);

    for (uint8_t bp_i = 0; bp_i < 7; bp_i++)
    {
        for (uint32_t i = 0; i < (*bp_size); i++)
        {
            bp_row[i] = bp_row[i] ^ bp_row_next[i];
        }
        bp_row += (*bp_size);
        bp_row_next += (*bp_size);
    }

    return bps;
}

uint8_t *bit_plane_unslice_8(uint8_t *bps, uint16_t w, uint16_t h)
{
    uint32_t data_size = (uint32_t)w * h;

    uint32_t bp_row_size = (w + 7) / 8;
    uint32_t bp_size = bp_row_size * (uint32_t)h;

    uint8_t *data = malloc(data_size);
    memset(data, 0, data_size);

    uint8_t *bp_row = bps + 6 * bp_size;

    uint8_t *bp_row_next = bps + 7 * bp_size;

    for (uint8_t bp_i = 7; bp_i > 0; bp_i--)
    {
        for (uint32_t i = 0; i < bp_size; i++)
        {
            bp_row[i] = bp_row[i] ^ bp_row_next[i];
        }
        bp_row -= bp_size;
        bp_row_next -= bp_size;
    }

    bp_row = bps;

    for (uint8_t bp = 0; bp < 8; bp++)
    {
        uint8_t *data_p = data;
        for (uint16_t y = 0; y < h; y++)
        {
            for (uint16_t x = 0; x < w; x++)
            {
                *data_p |= ((bp_row[x / 8] >> (x % 8)) & 0x1) << bp;
                data_p++;
            }
            bp_row += bp_row_size;
        }
    }

    return data;
}