#include "types.h"
#include "mort.h"
#include "utils.h"
#include <string.h>

enum
{
    QUAD_NW = 0,
    QUAD_NE,
    QUAD_SW,
    QUAD_SE,
    QUAD_Cnt
};

typedef struct
{
    u8 val;
    bool is_leaf;
} qtir_node;

u8 calc_levels(u16 w, u16 h)
{
    u8 lvls = 1;
    while ((1 << (lvls - 1)) < w || (1 << (lvls - 1)) < h)
    {
        lvls++;
    }

    return lvls;
}

u32 calc_node_count(u8 lvls)
{
    return 0x55555555 >> (32 - 2 * lvls);
}

qtir_node *qtir_from_pix(const u8 *data, u16 w, u16 h, u32 *qt_n)
{
    *qt_n = calc_node_count(calc_levels(w, h));

    u32 ir_size = sizeof(qtir_node) * (*qt_n);
    qtir_node *ir = malloc(ir_size);
    memset(ir, 0, ir_size);

    u32 mrtn = 0;
    qtir_node *base_nodes = ir + (*qt_n) / 4;

    const u8 *pixel = data;

    for (u16 y = 0; y < h; y++)
    {
        for (u16 x = 0; x < w; x++)
        {
            if (*pixel != 0)
            {
                base_nodes[mrtn].is_leaf = true;
                base_nodes[mrtn].val = *pixel;
            }
            pixel++;
            morton_inc_x(&mrtn);
        }

        morton_rst_x(&mrtn);
        morton_inc_y(&mrtn);
    }

    qtir_node *ir_p, *ir_c;

    ir_c = ir + *qt_n - 1;
    ir_p = ir + *qt_n / 4 - 1;

    while (ir_c > ir)
    {
        u8 pv = 0;

        for (u8 q = 0; q < QUAD_Cnt; q++)
        {
            if (ir_c->val != 0)
            {
                pv |= 1 << (3 - q);
            }

            ir_c--;
        }

        ir_p->val = pv;

        ir_p--;
    }

    return ir;
}

void qtir_get_fills(qtir_node *ir, u32 qt_n)
{
    qtir_node *ir_p, *ir_c;

    ir_c = ir + qt_n - 1;
    ir_p = ir + qt_n / 4 - 1;

    while (ir_c > ir)
    {
        if (ir_p->val == 0xF)
        {
            bool all_equal_leaves = true;

            u8 cv = ir_c->val;

            for (u8 q = 0; q < QUAD_Cnt; q++)
            {
                if (ir_c->val != cv || !ir_c->is_leaf)
                {
                    all_equal_leaves = false;
                }
                ir_c--;
            }

            if (all_equal_leaves)
            {
                ir_p->is_leaf = true;
                ir_p->val = cv;
            }
        }
        else
        {
            ir_c -= 4;
        }

        ir_p--;
    }
}

void qt_fill_ones(u8 *qt, u32 i, u32 qt_n)
{
    u32 lvl_size = 1;
    while (i < qt_n)
    {
        for (u32 j = 0; j < lvl_size; j++)
        {
            na_write(qt, i + j, 0xF);
        }

        i = 4 * i + 1;
        lvl_size *= 4;
    }
}

u8 *qtir_compress(qtir_node *ir, u32 qt_n, u32 *out_size)
{
    u32 qtc_size = (qt_n / 2 + qt_n);

    u8 *qtc = malloc(qtc_size);
    memset(qtc, 0, qtc_size);

    u32 skip_nodes_size = (qt_n + 1) / 2;
    u8 *skip_nodes = malloc(skip_nodes_size);
    memset(skip_nodes, 0, skip_nodes_size);

    u32 qtc_i = 0;
    u32 ir_c = 0;

    while (ir_c < qt_n)
    {
        if (na_read(skip_nodes, ir_c) == 0 && ir[ir_c].val != 0)
        {
            u8 v = ir[ir_c].val;
            if (ir[ir_c].is_leaf)
            {
                na_write(qtc, qtc_i++, 0);
                na_write(qtc, qtc_i++, (v >> 4) & 0xF);
                na_write(qtc, qtc_i++, (v >> 0) & 0xF);

                qt_fill_ones(skip_nodes, ir_c, qt_n);
            }
            else
            {
                na_write(qtc, qtc_i++, v);
            }
        }

        ir_c++;
    }

    *out_size = (qtc_i + 1) / 2;
    free(skip_nodes);
    return realloc(qtc, *out_size);
}

u8 *qtc8b_encode(const u8 *data, u16 w, u16 h, u32 *out_size)
{
    u32 qt_n;
    qtir_node *ir = qtir_from_pix(data, w, h, &qt_n);
    qtir_get_fills(ir, qt_n);

    u8 *qtc = qtir_compress(ir, qt_n, out_size);

    free(ir);
    return qtc;
}