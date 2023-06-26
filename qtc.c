#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "morton.h"
#include "test_assets/assets.h"
#include "qtc.h"

typedef enum
{
    QT_DIR_WEST = 0x0,
    QT_DIR_EAST = 0x1,
    QT_DIR_NORTH = 0x0,
    QT_DIR_SOUTH = 0x2,

    QT_DIR_cnt
} qt_dir_e;

typedef enum
{
    QT_QUAD_NW = QT_DIR_NORTH | QT_DIR_WEST,
    QT_QUAD_NE = QT_DIR_NORTH | QT_DIR_EAST,
    QT_QUAD_SW = QT_DIR_SOUTH | QT_DIR_WEST,
    QT_QUAD_SE = QT_DIR_SOUTH | QT_DIR_EAST,

    QT_QUAD_cnt
} qt_quad_e;

typedef struct qt_ir_node_t
{
    struct qt_ir_node_t *quads[4];
} qt_ir_node_t;

uint8_t qt_ir_node_to_nib(qt_ir_node_t *node)
{
    uint8_t nib = 0;
    for (qt_quad_e quad = 0; quad < QT_QUAD_cnt; quad++)
    {
        nib |= (node->quads[quad] != NULL) << quad;
    }

    return nib;
}

void qt_ir_write_level(qt_ir_node_t *node, uint8_t *qtc, uint8_t lvl, size_t *i)
{
    if (node == NULL)
    {
        return;
    }

    if (lvl == 0)
    {
        uint8_t nib = qt_ir_node_to_nib(node);
        nibble_array_write(qtc, *i, nib);
        (*i)++;
    }
    else
    {
        for (qt_quad_e quad = 0; quad < QT_QUAD_cnt; quad++)
        {
            qt_ir_write_level(node->quads[quad], qtc, lvl - 1, i);
        }
    }
}

size_t qt_ir_count_internal_nodes(qt_ir_node_t *node, uint8_t r)
{
    if (node == NULL || r == 0)
    {
        return 0;
    }

    size_t sub_cnt = 1;

    for (qt_quad_e quad = 0; quad < QT_QUAD_cnt; quad++)
    {
        sub_cnt += qtc_calc_size(node->quads[quad], r - 1);
    }

    return sub_cnt;
}

void qt_ir_to_qtc(qt_ir_node_t *ir_root, uint8_t r, uint8_t **qtc)
{
    size_t int_node_cnt = qt_ir_count_internal_nodes(ir_root, r);

    size_t byte_cnt = (int_node_cnt + 1) / 2;

    *qtc = malloc(byte_cnt);
    size_t qtc_i = 0;

    for (uint8_t lvl = 0; lvl < r; lvl++)
    {
        qt_ir_write_level(ir_root, *qtc, lvl, &qtc_i);
    }
}

/**
 * Write a value to an emulated nibble array
 *
 * @param p pointer to nibble array
 * @param i nibble index
 * @param val 4 bit value (4 lsbs are masked out)
 */
void nibble_array_write(uint8_t *p, size_t i, uint8_t val)
{
    size_t byte_index = i / 2;
    uint8_t nibble_shift = (i & 1) * 4;

    p[byte_index] &= ~(0xF << nibble_shift);
    p[byte_index] |= (val & 0xF) << nibble_shift;
}

/**
 * Read a value from an emulated nibble array
 *
 * @param p pointer to nibble array
 * @param i nibble index
 * @return 4 bit value at index i
 */
uint8_t nibble_array_read(uint8_t const *const p, size_t i)
{
    size_t byte_index = i / 2;
    uint8_t nibble_shift = (i & 1) * 4;

    return (p[byte_index] >> nibble_shift) & 0xF;
}

/**
 * Fill a subtree of a linear quadtree with 1s.
 *
 * @param qt pointer to quadtree array
 * @param qt_n node count of whole quadtree
 * @param i index of subtree's root node
 */
static void fill_ones(uint8_t *qt, size_t const qt_n, size_t i)
{
    size_t nodes_to_fill = 1;
    while (i < qt_n)
    {
        for (size_t j = 0; j < nodes_to_fill; j++)
        {
            nibble_array_write(qt, j + i, 0xF);
        }
        i *= 4;
        nodes_to_fill *= 4;
    }
}

void qtc_to_qt(uint8_t const *const qtc, uint8_t r, uint8_t const **qtd, size_t *qtd_n)
{
    // calc the number of nodes in a perfect quad tree with height r
    // # nodes = (4^r - 1) / 3
    size_t qt_n = ((1 << (2 * r + 2)) - 1) / 3;

    // allocate space for the decompressed quad tree
    uint8_t *qt = malloc((qt_n + 1) / 2);
    assert(qt != NULL);

    memset(qt, 0, qt_n);

    size_t qt_parent_i = 0;
    size_t qt_child_i = 0;
    size_t qtc_i = 0;

    // copy the first node of the compressed quad tree (the root)
    nibble_array_write(qt, qt_child_i++, nibble_array_read(qtc, qtc_i++));

    while (qt_child_i < qt_n)
    {
        // read one node
        uint8_t parent = nibble_array_read(qt, qt_parent_i);

        if (parent != 0 && nibble_array_read(qtc, qtc_i) == 0b0000)
        {
            qtc_i++;

            fill_ones(*qt, qt_n, qt_parent_i);
        }
        else
        {
            // write 4 nodes
            for (qt_quad_e quad = 0; quad < QT_QUAD_cnt; quad++)
            {
                if ((parent >> quad) & 0x1 && nibble_array_read(qt, qt_child_i) == 0)
                {
                    nibble_array_write(qt, qt_child_i, nibble_array_read(qtc, qtc_i++));
                }
                qt_child_i++;
            }
        }
        qt_parent_i++;
    }

    *qtd_n = qt_n;
    *qtd = qt;
}

void qt_ir_node_delete(qt_ir_node_t *node)
{
    if (node == NULL)
    {
        return;
    }

    for (uint8_t i = 0; i < QT_QUAD_cnt; i++)
    {
        qt_ir_node_delete(node->quads[i]);
    }

    free(node);
}

bool qt_ir_get_val(qt_ir_node_t *node, uint16_t x, uint16_t y, uint8_t r)
{
    if (r == 0)
    {
        return true;
    }

    bool all_null = true;
    for (qt_quad_e quad = 0; quad < QT_QUAD_cnt; quad++)
    {
        if (node->quads[quad] != NULL)
        {
            all_null = false;
            break;
        }
    }

    if (all_null)
    {
        return true;
    }

    r--;

    qt_quad_e quad = ((x >> r) & 0x1) | (((y >> r) & 0x1) << 1);

    if (node->quads[quad])
    {
        return qt_ir_get_val(node->quads[quad], x, y, r);
    }

    return false;
}

size_t qt_ir_nodes_count(qt_ir_node_t *node)
{
    if (node == NULL)
    {
        return 0;
    }

    size_t sub_count = 0;
    for (qt_quad_e quad = 0; quad < QT_QUAD_cnt; quad++)
    {
        sub_count += qt_ir_nodes_count(node->quads[quad]);
    }

    return 1 + sub_count;
}

qtc_ir_t *qt_ir_from_bmp(const uint8_t *bmp_arr, uint16_t pix_w, uint16_t pix_h)
{
    uint8_t r = 0;
    while ((1 << r) < pix_w)
    {
        r++;
    }

    while ((1 << r) < pix_h)
    {
        r++;
    }

    uint16_t qt_w = (1 << r);

    uint16_t bmp_w_bytes = (pix_w + 7) / 8;
    const uint8_t *row = bmp_arr;

    uint32_t leaf_cnt = qt_w * qt_w;

    qt_ir_node_t **lvl_nodes = malloc(sizeof(qt_ir_node_t *) * leaf_cnt);

    for (uint16_t y = 0; y < qt_w; y++)
    {
        for (uint16_t x = 0; x < qt_w; x++)
        {
            qt_ir_node_t *node = NULL;
            if (x < pix_w && y < pix_h)
            {
                if ((row[x / 8] >> (x % 8)) & 0x1)
                {
                    node = malloc(sizeof(qt_ir_node_t));
                    for (qt_quad_e quad = 0; quad < QT_QUAD_cnt; quad++)
                    {
                        node->quads[quad] = NULL;
                    }
                }
            }

            uint32_t pix_mc = xy_to_morton(x, y);
            lvl_nodes[pix_mc] = node;
        }

        row += bmp_w_bytes;
    }

    uint32_t lvl_width = leaf_cnt;

    while (lvl_width > 1)
    {
        uint32_t prev_lvl_width = lvl_width;
        lvl_width /= 4;
        qt_ir_node_t **prev_lvl_nodes = lvl_nodes;
        lvl_nodes = malloc(sizeof(qt_ir_node_t *) * lvl_width);

        for (uint32_t i = 0; i < lvl_width; i++)
        {
            qt_ir_node_t *node = NULL;

            bool child_not_null = false;
            for (qt_quad_e quad = 0; quad < QT_QUAD_cnt; quad++)
            {
                if (prev_lvl_nodes[quad] != NULL)
                {
                    child_not_null = true;
                    break;
                }
            }

            if (child_not_null)
            {
                node = malloc(sizeof(qt_ir_node_t));
                for (uint8_t quad = 0; quad < QT_QUAD_cnt; quad++)
                {
                    node->quads[quad] = *prev_lvl_nodes;
                    prev_lvl_nodes++;
                }
            }
            else
            {
                prev_lvl_nodes += QT_QUAD_cnt;
            }

            lvl_nodes[i] = node;
        }

        free(prev_lvl_nodes - prev_lvl_width);
    }

    qtc_ir_t *qt_ir = malloc(sizeof(qt_ir[0]));

    qt_ir->root = *lvl_nodes;
    qt_ir->r = r;
    qt_ir->w = pix_w;
    qt_ir->h = pix_h;

    return qt_ir;
}

bool qt_ir_compress(qt_ir_node_t *node, uint8_t r)
{
    if (node == NULL)
    {
        return false;
    }

    if (r == 0)
    {
        return true;
    }

    bool all_set = true;

    for (qt_quad_e quad = 0; quad < QT_QUAD_cnt; quad++)
    {
        if (!qt_ir_compress(node->quads[quad], r - 1))
        {
            all_set = false;
        }
    }

    if (all_set)
    {
        for (qt_quad_e quad = 0; quad < QT_QUAD_cnt; quad++)
        {
            qt_ir_node_delete(node->quads[quad]);
            node->quads[quad] = NULL;
        }
    }

    return all_set;
}

void qt_ir_print_img(qt_ir_node_t *node, uint8_t r)
{
    for (uint16_t y = 0; y < (1 << r); y++)
    {
        for (uint16_t x = 0; x < (1 << r); x++)
        {
            if (qt_ir_get_val(node, x, y, r))
            {
                printf("⬜");
            }
            else
            {
                printf("⬛");
            }
        }
        printf("\n");
    }
}

int main()
{
    qtc_ir_t *ir = qt_ir_from_bmp(globe_xxs_bits, GLOBE_XXS_WIDTH, GLOBE_XXS_HEIGHT);

    printf("r: %u\n", ir->r);
    printf("Nodes: %lu\n", qt_ir_nodes_count(ir->root));

    qt_ir_compress(ir->root, ir->r);
    printf("Nodes: %lu\n", qt_ir_nodes_count(ir->root));
    printf("%f\n", (GLOBE_XXS_WIDTH * GLOBE_XXS_HEIGHT / 8) / ((double)qt_ir_nodes_count(ir->root) / 2));

    lcqt_t *lcqt = qt_ir_to_lcqt(ir);
    printf("cr: %f\n", (GLOBE_XXS_WIDTH * GLOBE_XXS_HEIGHT / 8) / ((double)lcqt->data_size));
}