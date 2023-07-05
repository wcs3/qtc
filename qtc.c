#include "morton.h"
#include "qtir.h"
#include "utils.h"
#include "qtc.h"

#include <string.h>

/**
 * Calculate the number of nodes in a perfect quad tree with height r
 *
 * @param r height of quad tree
 * @return number of nodes
 */
static u32 qt_calc_node_cnt(u8 r)
{
    // # nodes = (4^r - 1) / 3
    return ((1 << (2 * r)) - 1) / 3;
}

static u8 qt_calc_r(u16 w, u16 h)
{
    u8 r = 0;
    while ((1 << (r + 1)) < w)
    {
        r++;
    }

    while ((1 << (r + 1)) < h)
    {
        r++;
    }

    return r;
}

void qt_set_rec(u8 *qt, u16 x, u16 y, u8 r, u32 i)
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
void qt_set(u8 *qt, u16 x, u16 y, u8 r)
{
    qt_set_rec(qt, x, y, r, 0);
}

void qt_from_pix(const u8 *pix, u16 w, u16 h, u8 **qt, u32 *qt_size)
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

    u32 qt_nibble_cnt = qt_calc_node_cnt(r);
    u32 qt_bytes = (qt_nibble_cnt + 1) / 2;

    *qt = malloc(qt_bytes);
    memset(*qt, 0, qt_bytes);

    u16 w_bytes = (w + 7) / 8;
    const u8 *pix_row = pix;

    for (u16 y = 0; y < h; y++)
    {
        for (u16 x = 0; x < w; x++)
        {
            if (bit_is_set(pix_row, x))
            {
                qt_set(qt, x, y, r);
            }
        }

        pix_row += w_bytes;
    }
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
    }
}

void qt_compress(u8 *qt, u32 qt_size, u8 r, u8 **qtc, u8 *qtc_size)
{
    *qtc = malloc(qt_size);

    u32 qt_n = qt_calc_node_cnt(r);

    u32 qt_p = 0;
    u32 qt_c = 0;

    u32 qtc_p_ofs = 0;
    u32 qtc_c = 0;

    na_write(*qtc, qtc_c++, na_read(qt, qt_c++));
    qtc_p_ofs++;

    while (qt_c < qt_n)
    {
        if (is_all_ones(qt, qt_p, qt_n))
        {
            na_write(*qtc, qtc_c++, 0);
            
            for (u8 q = 0; q < QUAD_Cnt; q++)
            {
                fill_zeros(qt, qt_c++, qt_n);
            }
            na_write(*qtc, qtc_c - qtc_p_ofs, COMP_CODE_FILL);
        }
        else
        {
            u8 qt_p = na_read(qt, qt_p);
            for (u8 q = 0; q < QUAD_Cnt; q++)
            {
                if (qt_p & (1 << q))
                {
                    na_write(*qtc, qtc_c++, na_read(qt, qt_c));
                    qtc_p_ofs++;
                }
                qt_c++;
            }
        }

        qt_p++;
    }

    *qtc_size = (qtc_c + 1) / 2;
    *qtc = realloc(*qtc, *qtc_size);
}

void qtc_encode(const u8 *pix, u16 w, u16 h, u8 **qtc, u32 *qtc_size)
{
    struct qtir_node *ir;
    u8 r;
    qtir_from_pix(pix, w, h, &ir, &r);
    qtir_compress(ir, r);
    qtir_linearize(ir, r, qtc, qtc_size);
    qtir_delete(ir);
}

/**
 * Fill a subtree of a linear quadtree with 1s.
 *
 * @param qt pointer to quadtree array
 * @param qt_n node count of whole quadtree
 * @param i index of subtree's root node
 */
static void fill_ones(u8 *qt, u32 n, u32 i)
{
    u32 len = 1;
    while (i < n)
    {
        for (u32 j = 0; j < len; j++)
        {
            na_write(qt, j + i, 0xF);
        }
        i *= 4;
        len *= 4;
    }
}

static void qtc_decompress(u8 *qtc, u8 r, u8 **qtd, u32 *qtd_n)
{
    u32 qt_n = qt_calc_node_cnt(r);

    // allocate space for the decompressed quad tree
    u8 *qt = malloc((qt_n + 1) / 2);
    assert(qt != NULL);

    memset(qt, 0, (qt_n + 1) / 2);

    u32 p_i = 0;
    u32 c_i = 0;
    u32 qtc_i = 0;

    // copy the first node of the compressed quad tree (the root)
    na_write(qt, c_i++, na_read(qtc, qtc_i++));

    while (c_i < qt_n)
    {
        u8 parent = na_read(qt, p_i);

        if (parent != 0 && na_read(qtc, qtc_i) == 0b0000)
        {
            qtc_i++;

            u8 x;
            u8 y = 0;

            switch (parent)
            {
            case COMP_CODE_FILL:
                fill_ones(qt, qt_n, p_i);
                break;
            case COMP_CODE_XXXY:
                y = na_read(qtc, qtc_i++);
            case COMP_CODE_XXX0:
                x = na_read(qtc, qtc_i++);
                na_write(qt, c_i++, y);
                na_write(qt, c_i++, x);
                na_write(qt, c_i++, x);
                na_write(qt, c_i++, x);
                break;

            case COMP_CODE_XXYX:
                y = na_read(qtc, qtc_i++);
            case COMP_CODE_XX0X:
                x = na_read(qtc, qtc_i++);
                na_write(qt, c_i++, x);
                na_write(qt, c_i++, y);
                na_write(qt, c_i++, x);
                na_write(qt, c_i++, x);
                break;

            case COMP_CODE_XYXX:
                y = na_read(qtc, qtc_i++);
            case COMP_CODE_X0XX:
                x = na_read(qtc, qtc_i++);
                na_write(qt, c_i++, x);
                na_write(qt, c_i++, x);
                na_write(qt, c_i++, y);
                na_write(qt, c_i++, x);
                break;

            case COMP_CODE_YXXX:
                y = na_read(qtc, qtc_i++);
            case COMP_CODE_0XXX:
                x = na_read(qtc, qtc_i++);
                na_write(qt, c_i++, x);
                na_write(qt, c_i++, x);
                na_write(qt, c_i++, x);
                na_write(qt, c_i++, y);
                break;

            case COMP_CODE_XXYY:
                y = na_read(qtc, qtc_i++);
                x = na_read(qtc, qtc_i++);
                na_write(qt, c_i++, y);
                na_write(qt, c_i++, y);
                na_write(qt, c_i++, x);
                na_write(qt, c_i++, x);
                break;

            case COMP_CODE_XYXY:
                y = na_read(qtc, qtc_i++);
                x = na_read(qtc, qtc_i++);
                na_write(qt, c_i++, y);
                na_write(qt, c_i++, x);
                na_write(qt, c_i++, y);
                na_write(qt, c_i++, x);
                break;

            case COMP_CODE_XYYX:
                y = na_read(qtc, qtc_i++);
                x = na_read(qtc, qtc_i++);
                na_write(qt, c_i++, x);
                na_write(qt, c_i++, y);
                na_write(qt, c_i++, y);
                na_write(qt, c_i++, x);
                break;

            case COMP_CODE_XXXX:
                x = na_read(qtc, qtc_i++);
                na_write(qt, c_i++, x);
                na_write(qt, c_i++, x);
                na_write(qt, c_i++, x);
                na_write(qt, c_i++, x);
                break;

            default:
                break;
            }
        }
        else
        {
            for (u8 q = 0; q < QUAD_Cnt; q++)
            {
                if ((parent >> q) & 0x1 && na_read(qt, c_i) == 0)
                {
                    na_write(qt, c_i, na_read(qtc, qtc_i++));
                }
                c_i++;
            }
        }
        p_i++;
    }

    *qtd_n = qt_n;
    *qtd = qt;
}

static void qt_to_pix(u8 *qt, u8 r, u16 w, u16 h, u8 **pix)
{
    u32 leaf_i = qt_calc_node_cnt(r - 1);

    u16 w_bytes = (w + 7) / 8;
    u32 bytes = (w_bytes * h);

    *pix = malloc(bytes);

    u8 *row = *pix;
    u32 mc = 0;

    for (u16 y = 0; y < h; y++)
    {
        for (u16 x = 0; x < w; x++)
        {
            if ((na_read(qt, leaf_i + mc / 4) >> (mc % 4)) & 0x1)
            {
                row[x / 8] |= 1 << (x % 8);
            }

            morton_inc_x(&mc);
        }
        morton_inc_y(&mc);
        morton_set_zero_x(&mc);
        row += w_bytes;
    }
}

void qtc_decode(u8 *qtc, u16 w, u16 h, u8 **pix)
{
    u8 r = qt_calc_r(w, h);
    u8 *qtd;
    u32 qtd_n;

    qtc_decompress(qtc, r, &qtd, &qtd_n);
    qt_to_pix(qtd, r, w, h, pix);

    free(qtd);
}