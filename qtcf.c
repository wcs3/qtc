#include "mort.h"
#include "utils.h"
#include "qtcf.h"
#include <string.h>

#include <stdio.h>

enum
{
    QUAD_NW = 0,
    QUAD_NE,
    QUAD_SW,
    QUAD_SE,
    QUAD_Cnt
};

enum
{
    QTC_HEADER_FLAG_INVERTED,
    QTC_HEADER_FLAG_ALL_BLACK,
};

/**
 * Quad tree intermediate representation type
 */
typedef struct
{
    u8 fill_height;
    u8 val;
    u32 sub_size;
} qtir_node;

/**
 * Calculate the number of nodes in a perfect quad tree
 *
 * @param lvls number of levels in quad tree
 * @return number of nodes
 */
static u32 calc_node_cnt(u8 lvls)
{
    // # nodes = (4^lvls - 1) / 3
    return 0x55555555 >> (32 - 2 * lvls);
}

/**
 * Calculate the height of quad tree needed to represent
 * an image of the specified size
 *
 * @param w width of image
 * @param h height of image
 *
 * @return number of levels needed for the quad tree
 */
static u8 calc_lvls(u16 w, u32 h)
{
    u8 lvls = 0;
    while ((u32)(1 << lvls) < w)
        lvls++;

    while ((u32)(1 << lvls) < h)
        lvls++;

    return lvls;
}

/**
 * Fill a subtree of an intermediate representation tree with ones (0xF) for
 * a number of levels
 *
 * @param qtir pointer to ir tree array
 * @param qt_i index of root node of subtree to fill
 * @param lvls number of levels to fill down from the subtree root
 */
static void qtir_fill_ones(qtir_node *qtir, u32 qt_i, u8 lvls)
{
    if (!lvls)
    {
        return;
    }

    for (u8 q = 0; q < QUAD_Cnt; q++)
    {
        qtir_fill_ones(qtir, 4 * qt_i + q + 1, lvls - 1);
    }

    qtir[qt_i].fill_height = (lvls - 1) % 9;
    qtir[qt_i].val = 0xF;
}

/**
 * Fill a subtree of a compact quad tree with a value at the specified level
 *
 * @param qt pointer to compacted qt array
 * @param qt_i index of root node of subtree to fill
 * @param lvls number of levels down from the subtree root where the
 * value is filled
 * @param val value to fill with
 */
static void qt_fill_val(u8 *qt, u32 qt_i, u8 lvls, u8 val)
{
    u32 lvl_len = 1;

    while (lvls)
    {
        for (u32 j = 0; j < lvl_len; j++)
        {
            na_write(qt, qt_i + j, 0xF);
        }

        lvls--;
        qt_i = 4 * qt_i + 1;
        lvl_len *= 4;
    }

    for (u32 j = 0; j < lvl_len; j++)
    {
        na_write(qt, qt_i + j, val);
    }
}

static void qtir_consolidate(qtir_node *qt, u32 qt_n)
{
    u32 qt_p, qt_c;

    qt_c = qt_n - 1;
    qt_p = qt_c / 4 - 1;

    while (qt_c > 0)
    {
        u8 v = 0;

        qtir_node *c = qt + qt_c - 3;
        for (u8 q = 0; q < QUAD_Cnt; q++)
        {
            if (c->val != 0)
            {
                v |= 1 << q;
            }
            c++;
        }
        qt[qt_p].val = v;
        qt_p--;
        qt_c -= 4;
    }
}

static qtir_node *qtir_from_raster(const u8 *data, u16 w, u16 h, u32 *qt_n)
{
    u8 qt_lvls = calc_lvls(w, h);

    if (qt_lvls == 0)
    {
        return NULL;
    }

    *qt_n = calc_node_cnt(qt_lvls);
    u32 qt_size = (*qt_n) * sizeof(qtir_node);

    qtir_node *qtir = (qtir_node *)malloc(qt_size);
    if (qtir == NULL)
    {
        return NULL;
    }

    u16 rast_row_size = (w + 7) / 8;
    const u8 *rast_row_hi = data;
    const u8 *rast_row_lo = rast_row_hi + rast_row_size;
    u16 rast_row_inc = 2 * rast_row_size;

    qtir_node *qt_leaves = qtir + *qt_n / 4;

    memset(qtir, 0, qt_size);

    u32 mc = 0;

    for (u16 y = 0; y < h - 1; y += 2)
    {
        for (u16 x = 0; x < w; x += 2)
        {
            u8 nwne = (rast_row_hi[x / 8] >> (x % 8)) & 0x3;
            u8 swse = (rast_row_lo[x / 8] >> (x % 8)) & 0x3;

            u8 nib = nwne | (swse << 2);

            if (nib != 0)
            {
                qt_leaves[mc].sub_size = 1;
            }

            qt_leaves[mc].val = nib;

            morton_inc_x(&mc);
        }

        morton_inc_y(&mc);
        morton_rst_x(&mc);

        rast_row_hi += rast_row_inc;
        rast_row_lo += rast_row_inc;
    }

    if (h & 1)
    {
        for (u16 x = 0; x < w; x += 2)
        {
            u8 nwne = (rast_row_hi[x / 8] >> (x % 8)) & 0x3;

            if (nwne != 0)
            {
                qt_leaves[mc].sub_size = 1;
            }

            qt_leaves[mc].val = nwne;
            morton_inc_x(&mc);
        }
    }

    qtir_consolidate(qtir, *qt_n);

    return qtir;
}

static void qtir_get_fills(qtir_node *qtir, u32 qt_n)
{
    qtir_node *qtir_p, *qtir_c;

    qtir_c = qtir + qt_n - 1;
    qtir_p = qtir + (qt_n - 1) / 4 - 1;

    while (qtir_c > qtir)
    {
        u8 v = qtir_c->val;
        u8 h = qtir_c->fill_height;
        qtir_c--;

        bool extend_fill = qtir_p->val == 0xF;

        for (u8 q = 1; q < QUAD_Cnt; q++)
        {
            if (extend_fill && (qtir_c->val != v || qtir_c->fill_height != h))
            {
                extend_fill = false;
            }

            qtir_c--;
        }

        if (extend_fill)
        {
            qtir_p->val = v;

            qtir_p->fill_height = h + 1;

            if (qtir_p->fill_height > 8)
            {
                qtir_p->fill_height = 0;
            }
        }

        qtir_p--;
    }

    qtir_c = qtir + qt_n / 16;
    qtir_p = qtir + qt_n / 64;

    while (qtir_c < qtir + qt_n)
    {
        if (qtir_p->fill_height == 0)
        {
            for (u8 q = 0; q < QUAD_Cnt; q++)
            {
                if (qtir_c->fill_height == 1 && qtir_c->val != 0xF)
                {
                    qtir_c->fill_height = 0;
                    qtir_c->val = 0xF;
                }
                qtir_c++;
            }
        }
        else
        {
            qtir_c += 4;
        }

        qtir_p++;
    }
}

static void qtir_get_sizes(qtir_node *qtir, u32 qt_n)
{
    qtir_node *qtir_p, *qtir_c, *lvl_start;

    qtir_c = qtir + qt_n - 1;
    qtir_p = qtir + (qt_n - 1) / 4 - 1;
    lvl_start = qtir_p;

    u8 h = 2;
    u32 lvl_len = qt_n - (qt_n - 1) / 4;
    lvl_len /= 4;
    u32 base_len = 4;

    while (qtir_c > lvl_start)
    {
        if (qtir_p->fill_height != 0 && qtir_p->val == 0xF)
        {
            qtir_p->sub_size = 1;
        }
        else if (qtir_p->val != 0)
        {
            qtir_p->sub_size = 1;

            for (u8 q = 0; q < QUAD_Cnt; q++)
            {
                if (qtir_p->val & (1 << q))
                {
                    qtir_p->sub_size += 1;
                }
            }
        }

        qtir_c -= 4;
        qtir_p--;
    }

    lvl_start = qtir_p;
    lvl_len /= 4;
    base_len *= 4;
    h++;

    while (qtir_c > lvl_start)
    {
        if (qtir_p->fill_height > 1)
        {
            qtir_p->sub_size = qtir_p->val == 0xF ? 2 : 3;
            qtir_c -= 4;
        }
        else if (qtir_p->fill_height == 1)
        {
            for (u8 q = 0; q < QUAD_Cnt; q++)
            {
                qtir_p->sub_size += qtir_c->sub_size;
                qtir_c--;
            }

            qtir_p->sub_size -= 4 - (qtir_p->val == 0xF ? 2 : 3);
        }
        else if (qtir_p->val != 0)
        {
            qtir_p->sub_size += 1;
            for (u8 q = 0; q < QUAD_Cnt; q++)
            {
                qtir_p->sub_size += qtir_c->sub_size;
                qtir_c--;
            }
        }
        else
        {
            qtir_c -= 4;
        }

        if (qtir_p->sub_size > base_len + 2)
        {
            qtir_p->val = 0xF;
            qtir_fill_ones(qtir, qtir_p - qtir, h - 1);
            qtir_p->sub_size = base_len + 2;
        }

        qtir_p--;
    }

    lvl_len /= 4;
    base_len *= 4;
    h++;

    u32 lvl_i = 0;

    while (qtir_c > qtir)
    {
        if (qtir_p->fill_height > 0)
        {
            for (u8 q = 0; q < QUAD_Cnt; q++)
            {
                qtir_p->sub_size += qtir_c->sub_size;
                qtir_c--;
            }

            if (qtir_p->fill_height == 1)
            {
                qtir_p->sub_size -= 4 - (qtir_p->val == 0xF ? 2 : 3);
            }
            else
            {
                qtir_p->sub_size -= 3 * (qtir_p->val == 0xF ? 2 : 3);
            }
        }
        else if (qtir_p->val != 0)
        {
            qtir_p->sub_size += 1;

            for (u8 q = 0; q < QUAD_Cnt; q++)
            {
                if (qtir_p->val & (1 << (3 - q)))
                {
                    qtir_p->sub_size += qtir_c->sub_size;
                }

                qtir_c--;
            }
        }
        else
        {
            qtir_c -= 4;
        }

        if (qtir_p->sub_size > base_len + 2)
        {
            qtir_p->val = 0xF;
            qtir_fill_ones(qtir, qtir_p - qtir, h - 1);
            qtir_p->sub_size = base_len + 2;
        }

        qtir_p--;
        lvl_i++;

        if (lvl_i >= lvl_len)
        {
            lvl_i = 0;
            lvl_len /= 4;
            base_len *= 4;
            h++;
        }
    }
}

static u8 *qtir_to_qtcf(qtir_node *qtir, u32 qt_n, bool inverted, u32 *out_size)
{
    qtir_get_fills(qtir, qt_n);
    qtir_get_sizes(qtir, qt_n);

    u32 qt_size = (qt_n + 2) / 2;
    u8 *qtcf = malloc(qt_size);
    memset(qtcf, 0, qt_size);

    u8 *skip_nodes = malloc(qt_size);
    memset(skip_nodes, 0, qt_size);

    u32 qtc_i = 0;

    u8 header = 0;

    header |= inverted << QTC_HEADER_FLAG_INVERTED;
    header |= (qtir[0].val == 0) << QTC_HEADER_FLAG_ALL_BLACK;

    na_write(qtcf, qtc_i++, header);

    u32 qtir_i = 0;

    u32 lvl_1_start = qt_n / 4;
    u32 lvl_2_start = lvl_1_start / 4;

    while (qtir_i < lvl_2_start)
    {
        qtir_node *p = qtir + qtir_i;

        if (na_read(skip_nodes, qtir_i) == 0)
        {
            if (p->fill_height > 0)
            {
                na_write(qtcf, qtc_i++, 0);

                if (p->val == 0xF)
                {
                    na_write(qtcf, qtc_i++, p->fill_height - 1);
                }
                else
                {
                    na_write(qtcf, qtc_i++, (p->fill_height - 1) | 0x8);
                    na_write(qtcf, qtc_i++, p->val);
                }

                qt_fill_val(skip_nodes, qtir_i, p->fill_height, 0xF);
            }
            else if (p->val != 0)
            {
                na_write(qtcf, qtc_i++, p->val);
            }
        }

        qtir_i++;
    }

    while (qtir_i < lvl_1_start)
    {
        qtir_node *p = qtir + qtir_i;

        if (na_read(skip_nodes, qtir_i) == 0)
        {
            if (p->fill_height != 0)
            {
                na_write(qtcf, qtc_i++, 0);
                qt_fill_val(skip_nodes, qtir_i, p->fill_height, 0xF);
            }
            else if (p->val != 0)
            {
                na_write(qtcf, qtc_i++, p->val);
            }
        }

        qtir_i++;
    }

    u8 q = 0;

    u32 pi = lvl_2_start;
    u8 pv = qtir[pi].val;

    while (qtir_i < qt_n)
    {
        qtir_node *p = qtir + qtir_i;

        if (na_read(skip_nodes, qtir_i) == 0 && (pv & (1 << q)))
        {
            na_write(qtcf, qtc_i++, p->val);
        }

        qtir_i++;
        q++;

        if ((qtir_i & 0x3) == 0x1)
        {
            q = 0;
            pi++;
            pv = qtir[pi].val;
        }
    }

    free(skip_nodes);

    *out_size = (qtc_i + 1) / 2;
    return realloc(qtcf, *out_size);
}

u8 *qtcf_encode(const u8 *data, u16 w, u32 h, u32 *out_size)
{
    qtir_node *qtir;
    u8 *qtcf, *qtcf_inv, *data_inv;
    u32 data_size, qtc_size, qtc_inv_size;
    u32 qt_n;

    qtir = qtir_from_raster(data, w, h, &qt_n);
    if (!qtir)
    {
        return NULL;
    }

    qtcf = qtir_to_qtcf(qtir, qt_n, false, &qtc_size);

    free(qtir);

    if (!qtcf)
    {
        return NULL;
    }

    data_size = (w + 7) / 8 * h;

    data_inv = malloc(data_size);
    if (!data_inv)
    {
        return NULL;
    }

    memcpy(data_inv, data, data_size);
    arr_invert(data_inv, data_size);

    qtir = qtir_from_raster(data_inv, w, h, &qt_n);
    free(data_inv);
    if (!qtir)
    {
        free(qtcf);
        return NULL;
    }

    qtcf_inv = qtir_to_qtcf(qtir, qt_n, true, &qtc_inv_size);

    free(qtir);
    if (!qtcf_inv)
    {
        free(qtcf);
        return NULL;
    }

    if (qtc_size <= qtc_inv_size)
    {
        free(qtcf_inv);
        *out_size = qtc_size;
    }
    else
    {
        free(qtcf);
        qtcf = qtcf_inv;
        *out_size = qtc_inv_size;
    }

    return qtcf;
}

static u8 *qtcf_to_qt(const u8 *qtc, u32 qt_n, bool *inverted, u32 *comp_size)
{
    u32 qt_size = (qt_n + 1) / 2;

    // allocate space for the decompressed quad tree
    u8 *qt = malloc(qt_size);
    assert(qt);

    memset(qt, 0, qt_size);

    u32 pi = ~0;
    u32 ci = 0;
    u32 qtc_i = 0;

    u8 header = na_read(qtc, qtc_i++);
    *inverted = (header >> 0) & 0x1;
    bool all_zero = (header >> 1) & 0x1;

    if (all_zero)
    {
        *comp_size = 1;
        return qt;
    }

    u32 lvl_1_start = qt_n / 4;
    u32 lvl_2_start = lvl_1_start / 4;

    u8 pv = 0x8;
    u8 q = 3;

    while (ci < lvl_2_start)
    {
        u8 cv = na_read(qt, ci);
        if (cv == 0 && pv & (1 << q))
        {
            if (na_read(qtc, qtc_i) == 0)
            {
                qtc_i++;

                u8 fh = na_read(qtc, qtc_i++);
                u8 fv = 0xF;
                if (fh & 0x8)
                {
                    fv = na_read(qtc, qtc_i++);
                    fh &= ~(0x8);
                }

                fh++;

                qt_fill_val(qt, ci, fh, fv);
            }
            else
            {
                na_write(qt, ci, na_read(qtc, qtc_i++));
            }
        }

        ci++;
        q++;

        if ((ci & 0x3) == 0x1)
        {
            pi++;
            q = 0;
            pv = na_read(qt, pi);
        }
    }

    while (ci < lvl_1_start)
    {
        u8 cv = na_read(qt, ci);
        if (cv == 0 && pv & (1 << q))
        {
            if (na_read(qtc, qtc_i) == 0)
            {
                qtc_i++;

                qt_fill_val(qt, ci, 1, 0xF);
            }
            else
            {
                na_write(qt, ci, na_read(qtc, qtc_i++));
            }
        }

        ci++;
        q++;

        if ((ci & 0x3) == 0x1)
        {
            pi++;
            q = 0;
            pv = na_read(qt, pi);
        }
    }

    while (ci < qt_n)
    {
        u8 cv = na_read(qt, ci);
        if (cv == 0 && pv & (1 << q))
        {
            na_write(qt, ci, na_read(qtc, qtc_i++));
        }

        ci++;
        q++;

        if ((ci & 0x3) == 0x1)
        {
            pi++;
            q = 0;
            pv = na_read(qt, pi);
        }
    }
    *comp_size = (qtc_i + 1) / 2;

    return qt;
}

static u8 *qt_to_raster(u8 *qt, u32 qt_n, u16 w, u16 h)
{
    u32 leaf_i = qt_n / 4;
    u16 rast_row_size = (w + 7) / 8;
    u32 raster_size = rast_row_size * h;

    u8 *raster = (u8 *)malloc(raster_size);
    memset(raster, 0, raster_size);

    u8 *rast_row_h = raster;
    u8 *rast_row_l = rast_row_h + rast_row_size;
    u32 rast_row_inc = 2 * rast_row_size;
    u32 mc = 0;

    for (u16 y = 0; y < h - 1; y += 2)
    {
        for (u16 x = 0; x < w; x += 2)
        {
            u8 nib = na_read(qt, leaf_i + mc);

            u8 nib_h = (nib >> 0) & 0x3;
            u8 nib_l = (nib >> 2) & 0x3;

            rast_row_h[x / 8] |= nib_h << (x % 8);
            rast_row_l[x / 8] |= nib_l << (x % 8);

            morton_inc_x(&mc);
        }
        morton_inc_y(&mc);
        morton_rst_x(&mc);
        rast_row_h += rast_row_inc;
        rast_row_l += rast_row_inc;
    }

    if (h & 1)
    {
        for (u16 x = 0; x < w; x += 2)
        {
            u8 nib = na_read(qt, leaf_i + mc);

            u8 nwne = nib & 0x3;

            rast_row_h[x / 8] |= nwne << (x % 8);

            morton_inc_x(&mc);
        }
    }

    return raster;
}

u8 *qtcf_decode(const u8 *qtc, u16 w, u16 h, u32 *comp_size)
{
    u8 lvls = calc_lvls(w, h);
    u32 qt_n = calc_node_cnt(lvls);

    bool inverted;

    u8 *qt = qtcf_to_qt(qtc, qt_n, &inverted, comp_size);

    u8 *pix = qt_to_raster(qt, qt_n, w, h);

    if (inverted)
    {
        arr_invert(pix, (w + 7) / 8 * h);
    }

    free(qt);

    return pix;
}