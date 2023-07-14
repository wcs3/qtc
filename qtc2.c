#include "mort.h"
#include "utils.h"
#include "qtc2.h"
#include <string.h>
#include <stdio.h>

typedef struct
{
    bool compressed;
    u8 code;
} qt_node;

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

static u8 calc_lvls(u16 w, u16 h)
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

static void fill_val(u8 *qt, u32 qt_i, u32 qt_n, u8 val)
{
    u32 lvl_len = 1;
    while (qt_i < qt_n)
    {
        for (u32 j = 0; j < lvl_len; j++)
        {
            na_write(qt, qt_i + j, val);
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
static void fill_zeros(qt_node *qt, u32 qt_i, const u32 qt_n)
{
    u32 lvl_len = 1;
    while (qt_i < qt_n)
    {
        for (u32 j = 0; j < lvl_len; j++)
        {
            qt[qt_i + j].code = 0;
            qt[qt_i + j].compressed = false;
        }

        lvl_len *= 4;
        qt_i *= 4;
        qt_i += 1;
    }
}

static void qt_set_rec(qt_node *qt, u16 x, u16 y, u8 r, u64 i)
{
    u8 q = ((x >> r) & 0x1) | (((y >> r) & 0x1) << 1);
    qt[i].code |= (1 << q);

    if (r > 0)
    {
        qt_set_rec(qt, x, y, r - 1, 4 * i + q + 1);
    }
    else
    {
        qt[i].compressed = true;
    }
}

// Set pixel at (x,y) in a quad tree
static void qt_set(qt_node *qt, u16 x, u16 y, u8 r)
{
    qt_set_rec(qt, x, y, r - 1, 0);
}

static void qt_from_pix(const u8 *pix, u16 w, u16 h, qt_node **qt, u32 *qt_n)
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
    u32 qt_size = (*qt_n) * sizeof(qt_node);

    *qt = (qt_node *)malloc(qt_size);
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

void parent_code_from_children(qt_node *nw, qt_node *ne, qt_node *sw, qt_node *se, qt_node *parent)
{
    parent->code = 0;

    parent->compressed = nw->compressed && ne->compressed && sw->compressed && se->compressed &&
                         nw->code == ne->code && ne->code == sw->code && sw->code == se->code && nw->code != 0;

    if (parent->compressed)
    {
        parent->code = nw->code;
    }
    else
    {
        parent->code |= (nw->code != 0) << QUAD_NW;
        parent->code |= (ne->code != 0) << QUAD_NE;
        parent->code |= (sw->code != 0) << QUAD_SW;
        parent->code |= (se->code != 0) << QUAD_SE;
    }
}

static void qt_compress(qt_node *qt, u32 qt_n, u8 **qtc, u32 *qtc_size)
{
    u32 qt_size = (qt_n + 1) / 2;
    *qtc = malloc(qt_size);
    memset(*qtc, 0, qt_size);

    u32 qt_c = qt_n - 1;
    u32 qt_p = qt_c / 4 - 1;

    while (qt_c > 0)
    {
        parent_code_from_children(&qt[qt_c - 3], &qt[qt_c - 2], &qt[qt_c - 1], &qt[qt_c - 0], &qt[qt_p]);
        qt_p--;
        qt_c -= 4;
    }

    u32 qtc_c = 0;

    qt_c = 1;
    qt_p = 0;

    na_write(*qtc, qtc_c++, qt[qt_p].code);

    while (qt_c < qt_n)
    {
        if (qt[qt_p].compressed)
        {
            na_write(*qtc, qtc_c++, 0);
            fill_zeros(qt, qt_p, qt_n);
            qt_c += 4;
        }
        else
        {
            for (u8 q = 0; q < QUAD_Cnt; q++)
            {
                if (qt[qt_p].code & (1 << q))
                {
                    na_write(*qtc, qtc_c++, qt[qt_c].code);
                }
                qt_c++;
            }
        }

        qt_p++;
    }

    *qtc_size = (qtc_c + 1) / 2;
    *qtc = realloc(*qtc, *qtc_size);
}

void qtc2_encode(const u8 *pix, u16 w, u16 h, u8 **qtc, u32 *qtc_size)
{
    qt_node *qt = NULL;
    u32 qt_n;
    qt_from_pix(pix, w, h, &qt, &qt_n);

    qt_compress(qt, qt_n, qtc, qtc_size);
    free(qt);
}

static void qtc_decompress(const u8 *qtc, u8 r, u8 **qt, u32 *qt_n, u32 *comp_size)
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

        if (na_read(*qt, qt_c) == 0 && parent != 0)
        {
            if (na_read(qtc, qtc_c) == 0)
            {
                qtc_c++;

                fill_val(*qt, qt_p, *qt_n, parent);
                qt_c += 4;
            }
            else
            {
                for (u8 q = 0; q < QUAD_Cnt; q++)
                {
                    if ((parent & (1 << q)) && na_read(*qt, qt_c) == 0)
                    {
                        na_write(*qt, qt_c, na_read(qtc, qtc_c++));
                    }
                    qt_c++;
                }
            }
        }
        else
        {
            qt_c += 4;
        }

        qt_p++;
    }

    *comp_size = (qtc_c + 7) / 8;
}

static void qt_to_pix(u8 *qt, u8 r, u16 w, u16 h, u8 **pix)
{
    u32 leaf_i = calc_node_cnt(r - 1);
    u16 w_bytes = (w + 7) / 8;
    u32 bytes = (w_bytes * h);

    *pix = (u8 *)malloc(bytes);
    memset(*pix, 0, bytes);

    u8 *row_h = *pix;
    u8 *row_l = row_h + w_bytes;
    u32 mc = 0;

    for (u16 y = 0; y < (h & ~1); y += 2)
    {
        for (u16 x = 0; x < w; x += 2)
        {
            u8 nib = na_read(qt, leaf_i + mc);

            u8 nib_h = (nib >> 0) & 0x3;
            u8 nib_l = (nib >> 2) & 0x3;

            row_h[x / 8] |= nib_h << (x % 8);
            row_l[x / 8] |= nib_l << (x % 8);

            morton_inc_x(&mc);
        }
        morton_inc_y(&mc);
        morton_set_zero_x(&mc);
        row_h += 2 * w_bytes;
        row_l += 2 * w_bytes;
    }

    if (h & 1)
    {
        for (uint16_t x = 0; x < w; x += 2)
        {
            uint8_t nib = na_read(qt, leaf_i + mc);

            uint8_t nwne = nib & 0x3;

            row_h[x / 8] |= nwne << (x % 8);

            morton_inc_x(&mc);
        }
    }
}

void qtc2_decode(const u8 *qtc, u16 w, u16 h, u8 **pix, u32 *comp_size)
{
    u8 r = calc_lvls(w, h);
    u8 *qt;
    u32 qt_n;

    qtc_decompress(qtc, r, &qt, &qt_n, comp_size);
    qt_to_pix(qt, r, w, h, pix);

    free(qt);
}