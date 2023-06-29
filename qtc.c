#include <stdio.h>

#include "morton.h"
#include "qtir.h"
#include "utils.h"
#include "qtc.h"

/**
 * Calculate the number of nodes in a perfect quad tree with height r
 *
 * @param r height of quad tree
 * @return number of nodes
 */
u32 qt_calc_node_cnt(u8 r)
{
    // # nodes = (4^r - 1) / 3
    return ((1 << (2 * r + 2)) - 1) / 3;
}

void qtc_encode(u8 *pix, u16 w, u16 h, u8 **qtc, u8 *qtc_n)
{
    struct qtir_node *ir;
    u8 r;
    pix_to_qtir(pix, w, h, &ir, &r);
    qtir_compress(ir, r);
    qtir_linearize(ir, r, qtc, qtc_n);
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

void qtc_decompress(u8 *qtc, u8 r, u8 **qtd, u32 *qtd_n)
{
    u32 qt_n = qt_calc_node_cnt(r);

    // allocate space for the decompressed quad tree
    u8 *qt = malloc((qt_n + 1) / 2);
    assert(qt != NULL);

    memset(qt, 0, qt_n);

    u32 p_i = 0;
    u32 c_i = 0;
    u32 qtc_i = 0;

    // copy the first node of the compressed quad tree (the root)
    na_write(qt, c_i++, na_read(qtc, qtc_i++));

    while (c_i < qt_n)
    {
        // read one node
        u8 parent = na_read(qt, p_i);

        if (parent != 0 && na_read(qtc, qtc_i) == 0b0000)
        {
            qtc_i++;

            fill_ones(*qt, qt_n, p_i);
        }
        else
        {
            // write 4 nodes
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

void qtc_decode(u8 *qtc, u8 r, u8 **pix)
{
}