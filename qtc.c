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

typedef struct qtir_node_t
{
    struct qtir_node_t *quads[4];
} qtir_node_t;

/**
 * Write a value to an emulated nibble array
 *
 * @param p pointer to nibble array
 * @param i nibble index
 * @param val 4 bit value (4 lsbs are masked out)
 */
static void na_write(uint8_t *p, size_t i, uint8_t val)
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
static uint8_t na_read(uint8_t const *const p, size_t i)
{
    size_t byte_index = i / 2;
    uint8_t nibble_shift = (i & 1) * 4;

    return (p[byte_index] >> nibble_shift) & 0xF;
}

static uint8_t qtir_node_to_nibble(qtir_node_t *node)
{
    uint8_t nib = 0;
    for (qt_quad_e quad = 0; quad < QT_QUAD_cnt; quad++)
    {
        nib |= (node->quads[quad] != NULL) << quad;
    }

    return nib;
}

static void qtir_linearize_level(qtir_node_t *node, uint8_t lvl, uint8_t *qtl, size_t *i)
{
    if (node == NULL)
    {
        return;
    }

    if (lvl == 0)
    {
        uint8_t nib = qtir_node_to_nibble(node);
        na_write(qtl, *i, nib);
        (*i)++;
    }
    else
    {
        for (qt_quad_e quad = 0; quad < QT_QUAD_cnt; quad++)
        {
            qtir_linearize_level(node->quads[quad], qtl, lvl - 1, i);
        }
    }
}

static size_t qtir_count_int_nodes(qtir_node_t *node, uint8_t r)
{
    if (node == NULL || r == 0)
    {
        return 0;
    }

    size_t sub_cnt = 1;

    for (qt_quad_e quad = 0; quad < QT_QUAD_cnt; quad++)
    {
        sub_cnt += qtir_count_int_nodes(node->quads[quad], r - 1);
    }

    return sub_cnt;
}

void qtir_linearize(qtir_node_t *ir_root, uint8_t r, uint8_t **qtl)
{
    size_t int_node_cnt = qtir_count_int_nodes(ir_root, r);

    size_t byte_cnt = (int_node_cnt + 1) / 2;

    *qtl = malloc(byte_cnt);
    size_t qtl_i = 0;

    for (uint8_t lvl = 0; lvl < r; lvl++)
    {
        qtir_linearize_level(ir_root, lvl, *qtl, &qtl_i);
    }
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
            na_write(qt, j + i, 0xF);
        }
        i *= 4;
        nodes_to_fill *= 4;
    }
}

void qtc_decompress(uint8_t const *const qtc, uint8_t r, uint8_t const **qtd, size_t *qtd_nodes)
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
    na_write(qt, qt_child_i++, na_read(qtc, qtc_i++));

    while (qt_child_i < qt_n)
    {
        // read one node
        uint8_t parent = na_read(qt, qt_parent_i);

        if (parent != 0 && na_read(qtc, qtc_i) == 0b0000)
        {
            qtc_i++;

            fill_ones(*qt, qt_n, qt_parent_i);
        }
        else
        {
            // write 4 nodes
            for (qt_quad_e quad = 0; quad < QT_QUAD_cnt; quad++)
            {
                if ((parent >> quad) & 0x1 && na_read(qt, qt_child_i) == 0)
                {
                    na_write(qt, qt_child_i, na_read(qtc, qtc_i++));
                }
                qt_child_i++;
            }
        }
        qt_parent_i++;
    }

    *qtd_nodes = qt_n;
    *qtd = qt;
}

void qtir_node_delete(qtir_node_t *node)
{
    if (node == NULL)
    {
        return;
    }

    for (uint8_t i = 0; i < QT_QUAD_cnt; i++)
    {
        qtir_node_delete(node->quads[i]);
    }

    free(node);
}

/**
 * Convert contiguous array of pixels to quad tree intermediate representation
 */
void pix_to_qtir(const uint8_t *pix, uint16_t w, uint16_t h, qtir_node_t **root, uint8_t *height)
{
}

/**
 * Convert array of pixels in byte-padded rows to quad tree intermediate representation
 */
void pix_bpr_to_qtir(const uint8_t *pix, uint16_t w, uint16_t h, qtir_node_t **root, uint8_t *height)
{
    uint8_t r = 0;
    while ((1 << r) < w)
    {
        r++;
    }

    while ((1 << r) < h)
    {
        r++;
    }

    uint16_t qt_w = (1 << r);
    uint32_t leaf_cnt = qt_w * qt_w;

    qtir_node_t **parent_lvl_nodes = malloc(sizeof(qtir_node_t *) * leaf_cnt);

    uint16_t w_bytes = (w + 7) / 8;
    const uint8_t *row = pix;

    uint32_t morton = 0;

    // populate leaves
    for (uint16_t y = 0; y < qt_w; y++, morton_inc_y(&morton), morton_set_zero_x(&morton))
    {
        for (uint16_t x = 0; x < qt_w; x++, morton_inc_x(&morton))
        {
            qtir_node_t *node = NULL;
            if (x < w && y < h)
            {
                if ((row[x >> 3] >> (x & 7)) & 0x1)
                {
                    node = malloc(sizeof(qtir_node_t));
                    for (qt_quad_e quad = 0; quad < QT_QUAD_cnt; quad++)
                    {
                        node->quads[quad] = NULL;
                    }
                }
            }
            parent_lvl_nodes[morton] = node;
        }

        row += w_bytes;
    }

    uint32_t parent_lvl_width = leaf_cnt;

    // build quad tree from leaves up to root
    while (parent_lvl_width > 1)
    {
        uint32_t child_lvl_width = parent_lvl_width;
        parent_lvl_width /= 4;

        qtir_node_t **child_lvl_nodes = parent_lvl_nodes;
        parent_lvl_nodes = malloc(sizeof(qtir_node_t *) * parent_lvl_width);

        size_t child_i = 0;

        for (uint32_t parent_i = 0; parent_i < parent_lvl_width; parent_i++)
        {
            qtir_node_t *node = NULL;

            bool child_not_null = false;
            for (qt_quad_e quad = 0; quad < QT_QUAD_cnt; quad++)
            {
                if (child_lvl_nodes[child_i + quad] != NULL)
                {
                    child_not_null = true;
                    break;
                }
            }

            if (child_not_null)
            {
                node = malloc(sizeof(qtir_node_t));
                for (uint8_t quad = 0; quad < QT_QUAD_cnt; quad++)
                {
                    node->quads[quad] = child_lvl_nodes[child_i];
                    child_i++;
                }
            }
            else
            {
                child_i += QT_QUAD_cnt;
            }

            parent_lvl_nodes[parent_i] = node;
        }

        free(child_lvl_nodes);
    }

    *root = parent_lvl_nodes[0];
    *height = r;

    free(parent_lvl_nodes);
}

bool qt_ir_compress(qtir_node_t *node, uint8_t r)
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
            qtir_node_delete(node->quads[quad]);
            node->quads[quad] = NULL;
        }
    }

    return all_set;
}

int main()
{
    qtc_ir_t *ir = qtir_from_pix(globe_xxs_bits, GLOBE_XXS_WIDTH, GLOBE_XXS_HEIGHT);

    printf("r: %u\n", ir->r);
    printf("Nodes: %lu\n", qtir_nodes_count(ir->root));

    qt_ir_compress(ir->root, ir->r);
    printf("Nodes: %lu\n", qtir_nodes_count(ir->root));
    printf("%f\n", (GLOBE_XXS_WIDTH * GLOBE_XXS_HEIGHT / 8) / ((double)qtir_nodes_count(ir->root) / 2));

    lcqt_t *lcqt = qt_ir_to_lcqt(ir);
    printf("cr: %f\n", (GLOBE_XXS_WIDTH * GLOBE_XXS_HEIGHT / 8) / ((double)lcqt->data_size));
}