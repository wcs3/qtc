#include "morton.h"
#include "utils.h"
#include "qtc.h"
#include <stdio.h>
#include <string.h>

/**
 * Calculate the number of nodes in a perfect quad tree with height r
 *
 * @param r height of quad tree
 * @return number of nodes
 */
static u32 calc_node_cnt(u8 r)
{
    // # nodes = (4^r - 1) / 3
    return ((1 << (2 * r)) - 1) / 3;
}

static u8 calc_r(u16 w, u16 h)
{
    u8 r = 0;
    while ((1 << r) < w)
    {
        r++;
    }

    while ((1 << r) < h)
    {
        r++;
    }

    return r;
}

static bool is_all_ones(u8 *qt, u32 qt_i, const u32 qt_n)
{
    bool all_set = na_read(qt, qt_i) == 0xF;

    qt_i = 4 * qt_i + 1;

    if (qt_i < qt_n)
    {
        for (u8 q = 0; q < QUAD_Cnt && all_set; q++)
        {
            all_set = all_set && is_all_ones(qt, qt_i + q, qt_n);
        }
    }

    return all_set;
}

/**
 * Fill a subtree of a linear quadtree with 1s.
 *
 * @param qt pointer to quadtree array
 * @param qt_i index of subtree's root node
 * @param qt_n node count of whole quadtree
 */
static void fill_ones(u8 *qt, u32 qt_i, u32 qt_n)
{
    u32 lvl_len = 1;
    while (qt_i < qt_n)
    {
        for (u32 j = 0; j < lvl_len; j++)
        {
            na_write(qt, qt_i + j, 0xF);
        }

        lvl_len *= 4;
        qt_i *= 4;
        qt_i += 1;
    }
}

/**
 * Fill a subtree of a linear quadtree with 0s.
 *
 * @param qt pointer to quadtree array
 * @param qt_i index of subtree's root node
 * @param qt_n node count of whole quadtree
 */
static void fill_zeros(u8 *qt, u32 qt_i, const u32 qt_n)
{
    u16 lvl_len = 1;
    while (qt_i < qt_n)
    {
        for (u16 j = 0; j < lvl_len; j++)
        {
            na_write(qt, qt_i + j, 0x0);
        }

        lvl_len *= 4;
        qt_i *= 4;
        qt_i += 1;
    }
}

static void qt_set_rec(u8 *qt, u16 x, u16 y, u8 r, u32 i)
{
    u8 q = ((x >> r) & 0x1) | (((y >> r) & 0x1) << 1);
    u8 v = na_read(qt, i);

    na_write(qt, i, v | (1 << q));

    if (r > 0)
    {
        qt_set_rec(qt, x, y, r - 1, 4 * i + q + 1);
    }
}

// Set pixel at (x,y) in a quad tree
static void qt_set(u8 *qt, u16 x, u16 y, u8 r)
{
    qt_set_rec(qt, x, y, r - 1, 0);
}

static bool qt_is_set_rec(u8 *qt, u16 x, u16 y, u8 r, u32 i)
{
    u8 q = ((x >> r) & 0x1) | (((y >> r) & 0x1) << 1);
    u8 v = na_read(qt, i);

    bool set = false;

    if (v & (1 << q))
    {
        if (r > 0)
        {
            set = qt_is_set_rec(qt, x, y, r - 1, 4 * i + q + 1);
        }
        else
        {
            set = true;
        }
    }

    return set;
}

static bool qt_is_set(u8 *qt, u16 x, u16 y, u8 r)
{
    return qt_is_set_rec(qt, x, y, r - 1, 0);
}

void qt_from_pix(const u8 *pix, u16 w, u16 h, u8 **qt, u32 *qt_n)
{
    u8 r = 0;
    while ((1 << r) < w)
    {
        r++;
    }

    while ((1 << r) < h)
    {
        r++;
    }

    assert(r > 0);

    *qt_n = calc_node_cnt(r);
    u32 qt_size = (*qt_n + 1) / 2;

    *qt = (u8 *)malloc(qt_size);
    memset(*qt, 0, qt_size);

    u16 w_bytes = (w + 7) / 8;
    const u8 *pix_row = pix;

    for (u16 y = 0; y < h; y++)
    {
        for (u16 x = 0; x < w; x++)
        {
            if (bit_is_set(pix_row, x))
            {
                qt_set(*qt, x, y, r);
            }
        }

        pix_row += w_bytes;
    }
}

static void qt_compress(u8 *qt, u32 qt_n, u8 **qtc, u32 *qtc_n)
{
    u32 qt_size = (qt_n + 1) / 2;
    *qtc = malloc(qt_size);
    memset(*qtc, 0, qt_size);

    u32 qt_p = 0;
    u32 qt_c = 0;

    u32 qtc_c = 0;

    na_write(*qtc, qtc_c++, na_read(qt, qt_c++));

    while (qt_c < qt_n)
    {
        u8 parent = na_read(qt, qt_p);
        for (u8 q = 0; q < QUAD_Cnt; q++)
        {
            if (parent & (1 << q))
            {
                if (is_all_ones(qt, qt_c, qt_n))
                {
                    na_write(*qtc, qtc_c++, 0);
                    fill_zeros(qt, qt_c, qt_n);
                }
                else
                {
                    na_write(*qtc, qtc_c++, na_read(qt, qt_c));
                }
            }
            qt_c++;
        }

        qt_p++;
    }

    *qtc_n = qtc_c;
    u32 qtc_size = (*qtc_n + 1) / 2;
    *qtc = realloc(*qtc, qtc_size);
}

void qtc_encode(const u8 *pix, u16 w, u16 h, u8 **qtc, u32 *qtc_size)
{
    u8 *qt = NULL;
    u32 qt_n;
    qt_from_pix(pix, w, h, &qt, &qt_n);

    u32 qtc_n;
    qt_compress(qt, qt_n, qtc, &qtc_n);
    *qtc_size = (qtc_n + 1) / 2;

    free(qt);
}

static void qtc_decompress(u8 *qtc, u8 r, u8 **qt, u32 *qt_n)
{
    *qt_n = calc_node_cnt(r);
    u32 qt_size = (*qt_n + 1) / 2;

    // allocate space for the decompressed quad tree
    *qt = malloc(qt_size);
    assert(*qt != NULL);

    memset(*qt, 0, qt_size);

    u32 qt_p = 0;
    u32 qt_c = 0;
    u32 qtc_c = 0;

    // copy the first node of the compressed quad tree (the root)
    na_write(*qt, qt_c++, na_read(qtc, qtc_c++));

    while (qt_c < *qt_n)
    {
        u8 parent = na_read(*qt, qt_p);
        for (u8 q = 0; q < QUAD_Cnt; q++)
        {
            if ((parent & (1 << q)) && na_read(*qt, qt_c) == 0)
            {
                if (na_read(qtc, qtc_c) == 0)
                {
                    qtc_c++;
                    fill_ones(*qt, qt_c, *qt_n);
                }
                else
                {
                    na_write(*qt, qt_c, na_read(qtc, qtc_c++));
                }
            }
            qt_c++;
        }
        qt_p++;
    }
}

static void qt_to_pix(u8 *qt, u8 r, u16 w, u16 h, u8 **pix)
{
    u16 w_bytes = (w + 7) / 8;
    u32 bytes = (w_bytes * h);

    *pix = (u8 *)malloc(bytes);
    memset(*pix, 0, bytes);

    u8 *row = *pix;

    for (u16 y = 0; y < h; y++)
    {
        for (u16 x = 0; x < w; x++)
        {
            if (qt_is_set(qt, x, y, r))
            {
                row[x / 8] |= 1 << (x % 8);
            }
        }
        row += w_bytes;
    }
}

void qtc_decode(u8 *qtc, u16 w, u16 h, u8 **pix)
{
    u8 r = calc_r(w, h);
    u8 *qt;
    u32 qt_n;

    qtc_decompress(qtc, r, &qt, &qt_n);
    qt_to_pix(qt, r, w, h, pix);

    free(qt);
}