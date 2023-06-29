#include "qtir.h"

static u8 node_to_nibble(qtir_node *node);
static u32 count_int_nodes(qtir_node *node, u8 r);
static void linearize_lvl(qtir_node *node, u8 lvl, u8 *qtl, u32 *i);

void qtir_from_pix(u8 *pix, u16 w, u16 h, qtir_node **ir, u8 *height)
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

    u16 qt_w = (1 << r);
    u32 leaf_cnt = qt_w * qt_w;

    qtir_node **pnodes = malloc(sizeof(qtir_node *) * leaf_cnt);

    u16 w_bytes = (w + 7) / 8;
    u8 *pix_row = pix;

    u32 morton = 0;

    // populate leaves
    for (u16 y = 0; y < qt_w; y++)
    {
        for (u16 x = 0; x < qt_w; x++)
        {
            qtir_node *node = NULL;
            if (x < w && y < h)
            {
                if (bit_is_set(pix_row, x))
                {
                    node = malloc(sizeof(qtir_node));

                    for (u8 q = 0; q < QUAD_Cnt; q++)
                    {
                        node->quads[q] = NULL;
                    }
                }
            }
            pnodes[morton] = node;
            morton_inc_x(&morton);
        }

        pix_row += w_bytes;
        morton_inc_y(&morton);
        morton_set_zero_x(&morton);
    }

    u32 p_w = leaf_cnt;

    // build quad tree from leaves up to root
    while (p_w > 1)
    {
        u32 c_w = p_w;
        p_w /= 4;

        qtir_node **cnodes = pnodes;
        pnodes = malloc(sizeof(qtir_node *) * p_w);

        u32 c_i = 0;

        for (u32 p_i = 0; p_i < p_w; p_i++)
        {
            qtir_node *node = NULL;

            bool child_not_null = false;
            for (u8 q = 0; q < QUAD_Cnt; q++)
            {
                if (cnodes[c_i + q] != NULL)
                {
                    child_not_null = true;
                    break;
                }
            }

            if (child_not_null)
            {
                node = malloc(sizeof(qtir_node));
                for (u8 q = 0; q < QUAD_Cnt; q++)
                {
                    node->quads[q] = cnodes[c_i];
                    c_i++;
                }
            }
            else
            {
                c_i += QUAD_Cnt;
            }

            pnodes[p_i] = node;
        }

        free(cnodes);
    }

    *ir = pnodes[0];
    *height = r;

    free(pnodes);
}

void qtir_delete(qtir_node *node)
{
    if (node == NULL)
    {
        return;
    }

    for (u8 q = 0; q < QUAD_Cnt; q++)
    {
        qtir_delete(node->quads[q]);
    }

    free(node);
}

u8 get_code(qtir_node *node)
{
    u8 child_cnt = 0;
    for (u8 q = 0; q < QUAD_Cnt; q++)
    {
        if (node->quads[q] != NULL)
        {
            child_cnt++;
        }
    }

    u8 code = COMP_CODE_NONE;

    if (child_cnt > 2)
    {
        u8 child_codes[4] = {0};
        for (u8 q = 0; q < QUAD_Cnt; q++)
        {
            qtir_node *child = node->quads[q];
            u8 child_code = COMP_CODE_NONE;
            if (child != NULL)
            {
                child_code = child->code;
                if (child_code == COMP_CODE_NONE)
                {
                    for (u8 cq = 0; cq < QUAD_Cnt; cq++)
                    {
                        if (child->quads[cq] != NULL)
                        {
                            child_code |= 1 << cq;
                        }
                    }
                }
            }
            child_codes[q] = child_code;
        }
    }

    return code;
}

void qtir_compress(qtir_node *node, u8 r)
{
    if (node == NULL)
    {
        return;
    }

    if (r == 0)
    {
        node->code = COMP_CODE_FILL;
        return;
    }

    bool all_set = true;

    for (u8 q = 0; q < QUAD_Cnt; q++)
    {
        qtir_compress(node->quads[q], r - 1);
        if (node->quads[q]->code != COMP_CODE_FILL)
        {
            all_set = false;
        }
    }

    if (all_set)
    {
        for (u8 q = 0; q < QUAD_Cnt; q++)
        {
            qtir_delete(node->quads[q]);
            node->quads[q] = NULL;
        }

        node->code = COMP_CODE_FILL;
    }
    else
    {
    }
}

void qtir_linearize(qtir_node *ir, u8 r, u8 **qtl, u32 *qtl_n)
{
    u32 nibble_cnt = count_int_nodes(ir, r);

    u32 byte_cnt = (nibble_cnt + 1) / 2;

    *qtl = malloc(byte_cnt);
    u32 qtl_i = 0;

    for (u8 lvl = 0; lvl < r; lvl++)
    {
        linearize_lvl(ir, lvl, *qtl, &qtl_i);
    }

    *qtl_n = byte_cnt;
}

static u8 node_to_nibble(qtir_node *node)
{
    u8 nib = 0;
    for (u8 q = 0; q < QUAD_Cnt; q++)
    {
        nib |= (node->quads[q] != NULL) << q;
    }

    return nib;
}

static u32 count_int_nodes(qtir_node *node, u8 r)
{
    if (node == NULL || r == 0)
    {
        return 0;
    }

    u32 sub_cnt = 1;

    for (u8 q = 0; q < QUAD_Cnt; q++)
    {
        sub_cnt += count_int_nodes(node->quads[q], r - 1);
    }

    return sub_cnt;
}

static void linearize_lvl(qtir_node *node, u8 lvl, u8 *qtl, u32 *i)
{
    if (node == NULL)
    {
        return;
    }

    if (lvl == 0)
    {
        u8 nib = node_to_nibble(node);
        na_write(qtl, *i, nib);
        (*i)++;
    }
    else
    {
        for (u8 q = 0; q < QUAD_Cnt; q++)
        {
            linearize_lvl(node->quads[q], qtl, lvl - 1, i);
        }
    }
}