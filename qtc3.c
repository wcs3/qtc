#include "mort.h"
#include "utils.h"
#include "qtc3.h"

#include <stdlib.h>
#include <string.h>

enum
{
    QUAD_NW = 0,
    QUAD_NE,
    QUAD_SW,
    QUAD_SE,
    QUAD_Cnt
};

/**
 * Calculate the number of nodes in a perfect quad tree with the
 * given number of levels
 *
 * @param r height of quad tree
 * @return number of nodes
 */
static uint32_t calc_node_cnt(uint8_t lvls)
{
    // # nodes = (4^r - 1) / 3
    return ((1 << (2 * lvls)) - 1) / 3;
}

/**
 * Calculate the number of levels needed for an uncompressed
 * quad tree to represent an image with the given width and height.
 *
 * @param w image width
 * @param h image height
 *
 * @return number of levels
 */
static uint8_t calc_lvls(uint16_t w, uint16_t h)
{
    uint8_t r = 0;
    while ((1 << r) < w)
        r++;

    while ((1 << r) < h)
        r++;

    return r;
}

static bool is_all_ones(uint8_t *qt, uint32_t qt_i, const uint32_t qt_n)
{
    bool all_set = na_read(qt, qt_i) == 0xF;

    qt_i = 4 * qt_i + 1;

    if (qt_i < qt_n)
    {
        for (uint8_t q = 0; q < QUAD_Cnt && all_set; q++)
        {
            all_set = all_set && is_all_ones(qt, qt_i + q, qt_n);
        }
    }

    return all_set;
}

/**
 * Fill a subtree of a linear quadtree with 0s.
 *
 * @param qt pointer to quadtree array
 * @param qt_i index of subtree's root node
 * @param qt_n node count of whole quadtree
 */
static void fill_zeros(uint8_t *qt, uint32_t qt_i, const uint32_t qt_n)
{
    uint32_t lvl_len = 1;
    while (qt_i < qt_n)
    {
        for (uint32_t j = 0; j < lvl_len; j++)
        {
            na_write(qt, qt_i + j, 0x0);
        }

        lvl_len *= 4;
        qt_i *= 4;
        qt_i += 1;
    }
}

/**
 * Fill a subtree of a linear quadtree with 1s.
 *
 * @param qt pointer to quadtree array
 * @param qt_i index of subtree's root node
 * @param qt_n node count of whole quadtree
 */
static void fill_ones(uint8_t *qt, uint32_t qt_i, const uint32_t qt_n)
{
    uint32_t lvl_len = 1;
    while (qt_i < qt_n)
    {
        for (uint32_t j = 0; j < lvl_len; j++)
        {
            na_write(qt, qt_i + j, 0xF);
        }

        lvl_len *= 4;
        qt_i *= 4;
        qt_i += 1;
    }
}

/**
 * Convert a 1-bit raster image to its uncompressed quad tree
 * representation
 *
 * @param pixels row-major pixel data
 * @param w image width
 * @param h image height
 * @param qt_n number of nodes in the uncompressed quad tree
 *
 * @return pointer to uncompressed quad tree. NULL if conversion unsuccessful
 */
static uint8_t *qt_from_pixels(const uint8_t *pixels, uint16_t w, uint16_t h, uint32_t *qt_n)
{
    uint8_t lvls = calc_lvls(w, h);

    if (lvls == 0)
    {
        return NULL;
    }

    *qt_n = calc_node_cnt(lvls);
    uint32_t qt_size = (*qt_n + 1) / 2;

    uint8_t *qt = (uint8_t *)malloc(qt_size);
    if (qt == NULL)
    {
        return NULL;
    }

    memset(qt, 0, qt_size);

    uint16_t px_row_bytes = (w + 7) / 8;
    const uint8_t *px_row_hi = pixels;
    const uint8_t *px_row_lo = px_row_hi + px_row_bytes;
    uint16_t px_row_inc = 2 * px_row_bytes;

    uint32_t leaf_pos = (*qt_n) / 4;

    uint32_t morton = 0;

    for (uint16_t y = 0; y < (h & ~1); y += 2)
    {
        for (uint16_t x = 0; x < w; x += 2)
        {
            uint8_t nwne = (px_row_hi[x / 8] >> (x % 8)) & 0x3;
            uint8_t swse = (px_row_lo[x / 8] >> (x % 8)) & 0x3;

            na_write(qt, leaf_pos + morton, nwne | (swse << 2));

            morton_inc_x(&morton);
        }

        morton_inc_y(&morton);
        morton_rst_x(&morton);

        px_row_hi += px_row_inc;
        px_row_lo += px_row_inc;
    }

    if (h & 1)
    {
        for (uint16_t x = 0; x < w; x += 2)
        {
            uint8_t nwne = (px_row_hi[x / 8] >> (x % 8)) & 0x3;

            na_write(qt, leaf_pos + morton, nwne);
            morton_inc_x(&morton);
        }
    }

    uint32_t chld_pos = *qt_n - 1;
    uint32_t prnt_pos = chld_pos / 4 - 1;

    while (chld_pos > 0)
    {
        uint8_t prnt = 0;

        for (uint8_t q = 0; q < QUAD_Cnt; q++)
        {
            if (na_read(qt, chld_pos) != 0)
            {
                prnt |= 1 << (3 - q);
            }
            chld_pos--;
        }
        na_write(qt, prnt_pos, prnt);
        prnt_pos--;
    }

    return qt;
}

/**
 * Compress a quad tree
 *
 * @param qt pointer to uncompressed quad tree
 * @param qt_n node count of uncompressed quad tree
 * @param out_size size of compressed quadtree in bytes
 *
 * @return pointer to compressed quad tree. NULL if unsuccessful
 */
static uint8_t *qt_compress(uint8_t *qt, uint32_t qt_n, bool inverted, uint32_t *out_size)
{
    uint32_t qt_size = (qt_n + 1) / 2;
    uint8_t *qtc = malloc(qt_size);
    if (qtc == NULL)
    {
        return NULL;
    }

    memset(qtc, 0, qt_size);

    uint32_t prnt_pos = 0;
    uint32_t chld_pos = 0;

    uint32_t qtc_pos = 0;

    na_write(qtc, qtc_pos++, inverted);
    na_write(qtc, qtc_pos++, na_read(qt, chld_pos++));

    while (chld_pos < qt_n)
    {
        uint8_t parent = na_read(qt, prnt_pos);
        for (uint8_t q = 0; q < QUAD_Cnt; q++)
        {
            if (parent & (1 << q))
            {
                if (is_all_ones(qt, chld_pos, qt_n))
                {
                    na_write(qtc, qtc_pos++, 0);
                    fill_zeros(qt, chld_pos, qt_n);
                }
                else
                {
                    na_write(qtc, qtc_pos++, na_read(qt, chld_pos));
                }
            }
            chld_pos++;
        }

        prnt_pos++;
    }

    *out_size = (qtc_pos + 1) / 2;
    return realloc(qtc, *out_size);
}

uint8_t *qtc3_encode(const uint8_t *data, uint16_t w, uint16_t h, uint32_t *out_size)
{
    uint8_t *qt, *qtc, *qtc_inv, *data_inv;
    uint32_t data_size, qtc_size, qtc_inv_size;
    uint32_t qt_n;

    data_size = (w + 7) / 8 * h;

    qt = qt_from_pixels(data, w, h, &qt_n);
    if (!qt)
    {
        return NULL;
    }

    qtc = qt_compress(qt, qt_n, false, &qtc_size);
    free(qt);

    if (!qtc)
    {
        return NULL;
    }

    data_inv = malloc(data_size);
    if (!data_inv)
    {
        return NULL;
    }

    memcpy(data_inv, data, data_size);
    arr_invert(data_inv, data_size);

    qt = qt_from_pixels(data_inv, w, h, &qt_n);
    free(data_inv);
    if (!qt)
    {
        free(qtc);
        return NULL;
    }

    qtc_inv = qt_compress(qt, qt_n, true, &qtc_inv_size);
    free(qt);
    if (!qtc_inv)
    {
        free(qtc);
        return NULL;
    }

    if (qtc_size < qtc_inv_size)
    {
        free(qtc_inv);
        *out_size = qtc_size;
    }
    else
    {
        free(qtc);
        qtc = qtc_inv;
        *out_size = qtc_inv_size;
    }

    return qtc;
}

static uint8_t *qtc_decompress(const uint8_t *qtc, uint16_t w, uint16_t h, bool *inverted, uint32_t *qt_n, uint32_t *comp_size)
{
    uint8_t r = calc_lvls(w, h);
    *qt_n = calc_node_cnt(r);
    uint32_t qt_size = (*qt_n + 1) / 2;

    // allocate space for the decompressed quad tree
    uint8_t *qt = malloc(qt_size);
    if (qt == NULL)
    {
        return NULL;
    }

    memset(qt, 0, qt_size);

    uint32_t prnt_pos = 0;
    uint32_t chld_pos = 0;
    uint32_t qtc_pos = 0;

    *inverted = na_read(qtc, qtc_pos++);

    // copy the first node of the compressed quad tree (the root)
    na_write(qt, chld_pos++, na_read(qtc, qtc_pos++));

    while (chld_pos < *qt_n)
    {
        uint8_t parent = na_read(qt, prnt_pos);
        for (uint8_t q = 0; q < QUAD_Cnt; q++)
        {
            if ((parent & (1 << q)) && na_read(qt, chld_pos) == 0)
            {
                if (na_read(qtc, qtc_pos) == 0)
                {
                    qtc_pos++;
                    fill_ones(qt, chld_pos, *qt_n);
                }
                else
                {
                    na_write(qt, chld_pos, na_read(qtc, qtc_pos++));
                }
            }
            chld_pos++;
        }
        prnt_pos++;
    }

    *comp_size = (qtc_pos + 1) / 2;

    return qt;
}

static uint8_t *qt_to_pixels(uint8_t *qt, uint32_t qt_n, uint16_t w, uint16_t h)
{
    uint32_t leaf_i = qt_n / 4;
    uint16_t px_row_size = (w + 7) / 8;
    uint32_t px_size = (px_row_size * h);

    uint8_t *pixels = (uint8_t *)malloc(px_size);
    memset(pixels, 0, px_size);

    uint8_t *px_row_hi = pixels;
    uint8_t *px_row_lo = px_row_hi + px_row_size;
    uint32_t morton = 0;

    for (uint16_t y = 0; y < (h & ~1); y += 2)
    {
        for (uint16_t x = 0; x < w; x += 2)
        {
            uint8_t nib = na_read(qt, leaf_i + morton);

            uint8_t nwne = (nib >> 0) & 0x3;
            uint8_t swse = (nib >> 2) & 0x3;

            px_row_hi[x / 8] |= nwne << (x % 8);
            px_row_lo[x / 8] |= swse << (x % 8);

            morton_inc_x(&morton);
        }
        morton_inc_y(&morton);
        morton_rst_x(&morton);
        px_row_hi += 2 * px_row_size;
        px_row_lo += 2 * px_row_size;
    }

    if (h & 1)
    {
        for (uint16_t x = 0; x < w; x += 2)
        {
            uint8_t nib = na_read(qt, leaf_i + morton);

            uint8_t nwne = nib & 0x3;

            px_row_hi[x / 8] |= nwne << (x % 8);

            morton_inc_x(&morton);
        }
    }

    return pixels;
}

uint8_t *qtc3_decode(const uint8_t *qtc, uint16_t w, uint16_t h, uint32_t *comp_size)
{
    uint8_t *qt, *pixels;
    uint32_t qt_n;
    bool inverted;

    qt = qtc_decompress(qtc, w, h, &inverted, &qt_n, comp_size);
    if (qt == NULL)
    {
        return NULL;
    }

    pixels = qt_to_pixels(qt, qt_n, w, h);
    free(qt);

    if (inverted)
    {
        arr_invert(pixels, (w + 7) / 8 * h);
    }

    return pixels;
}