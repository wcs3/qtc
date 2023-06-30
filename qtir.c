#include "qtir.h"
#include "utils.h"

static u8 node_to_nibble(qtir_node *node);
static u32 count_int_nodes(qtir_node *node, u8 r);
static void linearize_lvl(qtir_node *node, u8 lvl, u8 *qtl, u32 *i);

void qtir_from_pix(const u8 *pix, u16 w, u16 h, qtir_node **ir, u8 *height)
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
    const u8 *pix_row = pix;

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

/**
 * Calculate and the code for a given node
 *
 * @param node pointer to node
 */
void calc_code(qtir_node *node)
{
    u8 child_codes[4] = {COMP_CODE_NONE};
    u8 child_cnt = 0;

    u8 base_code = COMP_CODE_Cnt;

    for (u8 q = 0; q < QUAD_Cnt; q++)
    {
        if (node->quads[q] != NULL)
        {
            base_code |= 1 << q;
            child_codes[q] = node->quads[q]->code;
            child_cnt++;
        }
    }

    u8 code = COMP_CODE_NONE;

    // can only compress nodes with more than 2 children
    if (child_cnt > 2)
    {
        u8 nw = child_codes[QUAD_NW];
        u8 ne = child_codes[QUAD_NE];
        u8 sw = child_codes[QUAD_SW];
        u8 se = child_codes[QUAD_SE];

        if (ne == sw && sw == se)
        {
            if (nw == COMP_CODE_NONE)
            {
                code = COMP_CODE_XXX0;
            }
            else
            {
                code = COMP_CODE_XXXY;
            }
        }
        else if (nw == sw && sw == se)
        {
            if (ne == COMP_CODE_NONE)
            {
                code = COMP_CODE_XX0X;
            }
            else
            {
                code = COMP_CODE_XXYX;
            }
        }
        else if (nw == ne && ne == se)
        {
            if (sw == COMP_CODE_NONE)
            {
                code = COMP_CODE_X0XX;
            }
            else
            {
                code = COMP_CODE_XYXX;
            }
        }
        else if (nw == ne && ne == sw)
        {
            if (se == COMP_CODE_NONE)
            {
                code = COMP_CODE_0XXX;
            }
            else
            {
                code = COMP_CODE_YXXX;
            }
        }
        else if (nw == ne && sw == se)
        {
            code = COMP_CODE_XXYY;
        }
        else if (nw == sw && ne == se)
        {
            code = COMP_CODE_XYXY;
        }
        else if (nw == se && ne == sw)
        {
            code = COMP_CODE_XYYX;
        }
        else if (nw == ne && ne == sw && sw == se)
        {
            bool all_fill = nw == COMP_CODE_FILL;
            for (u8 q = 0; q < QUAD_Cnt && all_fill; q++)
            {
                if (!node->quads[q]->compressed)
                {
                    all_fill = false;
                }
            }

            if (all_fill)
            {
                code = COMP_CODE_FILL;
            }
            else
            {
                code = COMP_CODE_XXXX;
            }
        }
    }

    if (code != COMP_CODE_NONE)
    {
        node->code = code;
        node->compressed = true;
    }
    else
    {
        node->code = base_code;
        node->compressed = false;
    }
}

void qtir_compress(qtir_node *node, u8 r)
{
    if (node == NULL)
    {
        return;
    }

    if (r == 0)
    {
        node->compressed = true;
        node->code = COMP_CODE_FILL;
        return;
    }

    for (u8 q = 0; q < QUAD_Cnt; q++)
    {
        qtir_compress(node->quads[q], r - 1);
    }

    calc_code(node);

    if (node->compressed && node->code == COMP_CODE_FILL)
    {
        for (u8 q = 0; q < QUAD_Cnt; q++)
        {
            qtir_delete(node->quads[q]);
            node->quads[q] = NULL;
        }
    }
}

void qtir_linearize(qtir_node *ir, u8 r, u8 **qtl, u32 *qtl_size)
{
    u32 nibble_cnt = count_int_nodes(ir, r);

    u32 byte_cnt = (nibble_cnt + 1) / 2;

    *qtl = malloc(byte_cnt);
    u32 qtl_i = 0;

    for (u8 lvl = 0; lvl < r; lvl++)
    {
        linearize_lvl(ir, lvl, *qtl, &qtl_i);
    }

    *qtl_size = byte_cnt;
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
        na_write(qtl, *i, node->code);
        (*i)++;
    }
    else if (lvl == 1 && node->compressed)
    {
        // in parent of target level and compressed
        na_write(qtl, *i, 0);
        (*i)++;
        switch (node->code)
        {
        case COMP_CODE_FILL:
            break;

        case COMP_CODE_XXXY:
            na_write(qtl, *i, node->quads[QUAD_NW]);
            (*i)++;
        case COMP_CODE_XXX0:
            na_write(qtl, *i, node->quads[QUAD_NE]);
            (*i)++;
            break;

        case COMP_CODE_XXYX:
            na_write(qtl, *i, node->quads[QUAD_NE]);
            (*i)++;
        case COMP_CODE_XX0X:
            na_write(qtl, *i, node->quads[QUAD_SW]);
            (*i)++;
            break;

        case COMP_CODE_XYXX:
            na_write(qtl, *i, node->quads[QUAD_SW]);
            (*i)++;
        case COMP_CODE_X0XX:
            na_write(qtl, *i, node->quads[QUAD_SE]);
            (*i)++;
            break;

        case COMP_CODE_YXXX:
            na_write(qtl, *i, node->quads[QUAD_SE]);
            (*i)++;
        case COMP_CODE_0XXX:
            na_write(qtl, *i, node->quads[QUAD_NW]);
            (*i)++;
            break;

        case COMP_CODE_XXYY:
            na_write(qtl, *i, node->quads[QUAD_NW]);
            (*i)++;
            na_write(qtl, *i, node->quads[QUAD_SW]);
            (*i)++;
            break;

        case COMP_CODE_XYXY:
            na_write(qtl, *i, node->quads[QUAD_NW]);
            (*i)++;
            na_write(qtl, *i, node->quads[QUAD_NE]);
            (*i)++;
            break;
        case COMP_CODE_XYYX:
            na_write(qtl, *i, node->quads[QUAD_NE]);
            (*i)++;
            na_write(qtl, *i, node->quads[QUAD_NW]);
            (*i)++;
            break;

        case COMP_CODE_XXXX:
            na_write(qtl, *i, node->quads[QUAD_NW]);
            (*i)++;
            break;
        default:
            break;
        }
    }
    else
    {
        for (u8 q = 0; q < QUAD_Cnt; q++)
        {
            linearize_lvl(node->quads[q], lvl - 1, qtl, i);
        }
    }
}