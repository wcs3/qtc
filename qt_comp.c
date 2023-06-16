#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "vla.h"

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)       \
    ((byte)&0x80 ? '1' : '0'),     \
        ((byte)&0x40 ? '1' : '0'), \
        ((byte)&0x20 ? '1' : '0'), \
        ((byte)&0x10 ? '1' : '0'), \
        ((byte)&0x08 ? '1' : '0'), \
        ((byte)&0x04 ? '1' : '0'), \
        ((byte)&0x02 ? '1' : '0'), \
        ((byte)&0x01 ? '1' : '0')

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

typedef struct
{
    struct
    {
        uint32_t r : 4;
    } header;
    vla_t *qt_vla;
} lcqt_t;

uint32_t interleave_u16_zeroes(uint16_t u16)
{
    uint32_t u32 = u16;

    u32 = (u32 | (u32 << 8)) & 0b00000000111111110000000011111111;
    u32 = (u32 | (u32 << 4)) & 0b00001111000011110000111100001111;
    u32 = (u32 | (u32 << 2)) & 0b00110011001100110011001100110011;
    u32 = (u32 | (u32 << 1)) & 0b01010101010101010101010101010101;

    return u32;
}

uint32_t xy_to_morton(uint16_t x, uint16_t y)
{
    return interleave_u16_zeroes(x) | (interleave_u16_zeroes(y) << 1);
}

uint8_t *lqt_from_bitmap(uint8_t *pix_arr, uint8_t r)
{
    assert(r < 16);
    uint16_t w = 1 << r;

    size_t qt_size = ((1 << (2 * r + 2)) - 1) / 3;

    uint8_t *qt = malloc(qt_size * sizeof(qt[0]));
    memset(qt, 0, qt_size);

    uint8_t lvl = r;
    size_t lvl_width = w * w;
    uint8_t *lvl_start_p = qt + qt_size - (lvl_width);

    size_t byte_pos = 0;
    uint8_t bit_pos = 0;

    // populate leaves of quadtree with pixel data
    for (uint16_t y = 0; y < w; y++)
    {
        for (uint16_t x = 0; x < w; x++)
        {
            uint32_t mc = xy_to_morton(x, y);

            lvl_start_p[mc] = ((pix_arr[byte_pos] >> bit_pos) & 0x1) ? 0x0F : 0x00;

            bit_pos++;

            if (bit_pos == 8)
            {
                byte_pos++;
                bit_pos = 0;
            }
        }
    }

    lvl_width /= 4;

    // build quadtree from leaves up to root
    while (lvl_width > 0)
    {
        uint8_t *children_p = lvl_start_p;
        lvl_start_p -= lvl_width;

        uint8_t *lvl_p = lvl_start_p;

        for (uint32_t i = 0; i < lvl_width; i++, lvl_p++)
        {
            for (uint8_t quad = 0; quad < QT_QUAD_cnt; quad++)
            {
                *lvl_p |= (*children_p != 0) << quad;
                children_p++;
            }
        }

        lvl_width /= 4;
    }

    qt[0] |= (r << 4);

    return qt;
}

void lqt_print_rec(uint8_t *qt, size_t index, uint8_t curr_r, uint8_t r, qt_quad_e quad)
{
    if (curr_r == r || qt[index] == 0)
    {
        return;
    }

    for (uint8_t i = 0; i < curr_r; i++)
    {
        printf("\t");
    }

    switch (quad)
    {
    case QT_QUAD_NW:
        printf("NW: ");
        break;
    case QT_QUAD_NE:
        printf("NE: ");
        break;
    case QT_QUAD_SE:
        printf("SE: ");
        break;
    case QT_QUAD_SW:
        printf("SW: ");
        break;
    default:
        break;
    }

    printf(BYTE_TO_BINARY_PATTERN "\n", BYTE_TO_BINARY(qt[index]));

    for (uint8_t i = 0; i < QT_QUAD_cnt; i++)
    {
        lqt_print_rec(qt, 4 * index + i + 1, curr_r + 1, r, i);
    }
}

void lqt_print(uint8_t *lqt)
{
    assert(lqt);

    uint8_t r = lqt[0] >> 4;

    lqt_print_rec(lqt, 0, 0, r, QT_QUAD_cnt);
}

typedef struct qt_ir_node_t
{
    struct qt_ir_node_t *quads[4];
} qt_ir_node_t;

typedef struct
{
    qt_ir_node_t *root;
    bool invert;
    uint8_t r;
} qt_ir_t;

void qt_node_delete(qt_ir_node_t *node)
{
    if (node == NULL)
    {
        return;
    }

    for (uint8_t i = 0; i < QT_QUAD_cnt; i++)
    {
        qt_node_delete(node->quads[i]);
    }

    free(node);
}

bool qt_ir_node_get_val(qt_ir_node_t *node, uint16_t x, uint16_t y, uint8_t r)
{
    if (r == 0)
    {
        return true;
    }

    r--;

    qt_quad_e quad = ((x >> r) & 0x1) | (((y >> r) & 0x1) << 1);

    if (node->quads[quad])
    {
        return qt_ir_node_get_val(node->quads[quad], x, y, r);
    }

    return false;
}

bool qt_ir_get_val(qt_ir_t *ir, uint16_t x, uint16_t y)
{
    return qt_ir_node_get_val(ir->root, x, y, ir->r);
}

qt_ir_t *qt_ir_create(uint8_t *pix_arr, uint8_t r)
{
    uint16_t w = 1 << r;

    size_t pix_cnt = w * w;

    qt_ir_node_t **lvl_nodes = malloc(sizeof(qt_ir_node_t *) * pix_cnt);

    uint8_t bit = 0;

    // populate leaves with pixel values
    for (uint16_t y = 0; y < w; y++)
    {
        for (uint16_t x = 0; x < w; x++)
        {
            qt_ir_node_t *node = NULL;

            if ((*pix_arr >> bit) & 0x1)
            {
                node = malloc(sizeof(qt_ir_node_t));
                for (qt_quad_e quad = 0; quad < QT_QUAD_cnt; quad++)
                {
                    node->quads[quad] = NULL;
                }
            }

            size_t pix_mc = xy_to_morton(x, y);
            lvl_nodes[pix_mc] = node;

            bit++;

            if (bit == 8)
            {
                pix_arr++;
                bit = 0;
            }
        }
    }

    size_t lvl_width = pix_cnt;

    while (lvl_width > 1)
    {
        size_t prev_lvl_width = lvl_width;
        lvl_width /= 4;
        qt_ir_node_t **prev_lvl_nodes = lvl_nodes;
        lvl_nodes = malloc(sizeof(qt_ir_node_t *) * lvl_width);

        for (size_t i = 0; i < lvl_width; i++)
        {
            qt_ir_node_t *node = NULL;

            bool child_not_null = false;
            for (uint8_t quad = 0; quad < QT_QUAD_cnt; quad++)
            {
                child_not_null = child_not_null || (prev_lvl_nodes[quad] != NULL);
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

    qt_ir_t *qt_ir = malloc(sizeof(qt_ir[0]));

    qt_ir->root = *lvl_nodes;
    qt_ir->r = r;

    return qt_ir;
}

bool qt_ir_nodes_equal(qt_ir_node_t *a, qt_ir_node_t *b)
{
    if (a == NULL || b == NULL)
    {
        return a == b;
    }

    return qt_ir_nodes_equal(a->quads[0], b->quads[0]) &&
           qt_ir_nodes_equal(a->quads[1], b->quads[1]) &&
           qt_ir_nodes_equal(a->quads[2], b->quads[2]) &&
           qt_ir_nodes_equal(a->quads[3], b->quads[3]);
}

bool qt_ir_nodes_v_mirrored(qt_ir_node_t *a, qt_ir_node_t *b)
{
    if (a == NULL || b == NULL)
    {
        return a == b;
    }

    return qt_ir_nodes_v_mirrored(a->quads[QT_QUAD_NW], b->quads[QT_QUAD_NE]) &&
           qt_ir_nodes_v_mirrored(a->quads[QT_QUAD_NE], b->quads[QT_QUAD_NW]) &&
           qt_ir_nodes_v_mirrored(a->quads[QT_QUAD_SW], b->quads[QT_QUAD_SE]) &&
           qt_ir_nodes_v_mirrored(a->quads[QT_QUAD_SE], b->quads[QT_QUAD_SW]);
}

bool qt_ir_nodes_h_mirrored(qt_ir_node_t *a, qt_ir_node_t *b)
{
    if (a == NULL || b == NULL)
    {
        return a == b;
    }

    return qt_ir_nodes_h_mirrored(a->quads[QT_QUAD_NW], b->quads[QT_QUAD_SW]) &&
           qt_ir_nodes_h_mirrored(a->quads[QT_QUAD_SW], b->quads[QT_QUAD_NW]) &&
           qt_ir_nodes_h_mirrored(a->quads[QT_QUAD_NE], b->quads[QT_QUAD_SE]) &&
           qt_ir_nodes_h_mirrored(a->quads[QT_QUAD_SE], b->quads[QT_QUAD_NE]);
}

bool qt_ir_nodes_d1_mirrored(qt_ir_node_t *a, qt_ir_node_t *b)
{
    if (a == NULL || b == NULL)
    {
        return a == b;
    }

    return qt_ir_nodes_d1_mirrored(a->quads[QT_QUAD_NW], b->quads[QT_QUAD_SE]) &&
           qt_ir_nodes_d1_mirrored(a->quads[QT_QUAD_SE], b->quads[QT_QUAD_NW]) &&
           qt_ir_nodes_d1_mirrored(a->quads[QT_QUAD_NE], b->quads[QT_QUAD_NE]) &&
           qt_ir_nodes_d1_mirrored(a->quads[QT_QUAD_SW], b->quads[QT_QUAD_SW]);
}

bool qt_ir_nodes_d2_mirrored(qt_ir_node_t *a, qt_ir_node_t *b)
{
    if (a == NULL || b == NULL)
    {
        return a == b;
    }

    return qt_ir_nodes_d2_mirrored(a->quads[QT_QUAD_NE], b->quads[QT_QUAD_SW]) &&
           qt_ir_nodes_d2_mirrored(a->quads[QT_QUAD_SW], b->quads[QT_QUAD_NE]) &&
           qt_ir_nodes_d2_mirrored(a->quads[QT_QUAD_NW], b->quads[QT_QUAD_NW]) &&
           qt_ir_nodes_d2_mirrored(a->quads[QT_QUAD_SE], b->quads[QT_QUAD_SE]);
}



void qt_node_print_tree(qt_ir_node_t *node, uint8_t tabs, qt_quad_e quad)
{
    if (node != NULL)
    {
        for (uint8_t i = 0; i < tabs; i++)
        {
            printf("\t");
        }

        printf("%u ", tabs);

        switch (quad)
        {
        case QT_QUAD_NW:
            printf("NW: ");
            break;
        case QT_QUAD_NE:
            printf("NE: ");
            break;
        case QT_QUAD_SE:
            printf("SE: ");
            break;
        case QT_QUAD_SW:
            printf("SW: ");
            break;
        default:
            break;
        }

        printf("\n");

        for (uint8_t i = 0; i < QT_QUAD_cnt; i++)
        {
            qt_node_print_tree(node->quads[i], tabs + 1, i);
        }
    }
}

void qt_ir_print_tree(qt_ir_t *ir)
{
    if (ir->root)
    {
        for (uint8_t i = 0; i < QT_QUAD_cnt; i++)
        {
            qt_node_print_tree(ir->root->quads[i], 0, i);
        }
    }
}

void qt_ir_print_img(qt_ir_t *ir)
{
    uint16_t width = 1 << ir->r;

    for (uint16_t y = 0; y < width; y++)
    {
        for (uint16_t x = 0; x < width; x++)
        {
            if (qt_ir_get_val(ir, x, y))
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

typedef struct
{
    size_t byte_cnt;
    uint8_t data[];
} qt_comp_t;

void qt_comp_traverse(uint8_t *d, bool left_nibble, size_t right_nodes_cnt, int32_t x, int32_t y, int32_t half_w)
{
    uint8_t children = (*d >> (left_nibble * 4)) & 0x0F;

    if (!children)
    {
        return;
    }
}

void qt_comp_decompress(qt_comp_t *qtc, uint8_t **res, size_t res_n)
{
    size_t node_i = 0;

    uint8_t *d = malloc(sizeof(uint8_t));

    size_t alloced = 1;

    uint8_t bit_pos = 0;
    uint8_t mask = 0x0F;

    uint8_t node = (qtc->data[node_i / 2] >> (4 * (node_i & 1))) & mask;

    node_i++;

    while (node)
    {
        uint8_t node = (qtc->data[node_i / 2] >> (4 * (node_i & 1))) & mask;
        while (node)
        {
            if (node & 1)
            {
            }
        }

        node = (qtc->data[node_i / 2] >> (4 * (node_i & 1))) & mask;
        node_i++;
    }
}

uint8_t test_img[] = {0b00111100, 0b01000010, 0b10000001, 0b10000001, 0b10000001, 0b10000001, 0b01000010, 0b00111100};

uint8_t test_img2[] = {0b00111100, 0b01000010, 0b10000001, 0b10000001, 0b10000001, 0b10000001, 0b01000010, 0b00111100, 0b10000001, 0b10000001, 0b01000010, 0b00111100, 0b00111100, 0b01000010, 0b10000001, 0b10000001,
                       0b10000001, 0b10000001, 0b01000010, 0b00111100, 0b00111100, 0b01000010, 0b10000001, 0b10000001, 0b00111100, 0b01000010, 0b10000001, 0b10000001, 0b10000001, 0b10000001, 0b01000010, 0b00111100,
                       0b10000001, 0b10000001, 0b01000010, 0b00111100, 0b00111100, 0b01000010, 0b10000001, 0b10000001, 0b00111100, 0b01000010, 0b10000001, 0b10000001, 0b10000001, 0b10000001, 0b01000010, 0b00111100,
                       0b00111100, 0b01000010, 0b10000001, 0b10000001, 0b10000001, 0b10000001, 0b01000010, 0b00111100, 0b10000001, 0b10000001, 0b01000010, 0b00111100, 0b00111100, 0b01000010, 0b10000001, 0b10000001,
                       0b10000001, 0b10000001, 0b01000010, 0b00111100, 0b00111100, 0b01000010, 0b10000001, 0b10000001, 0b00111100, 0b01000010, 0b10000001, 0b10000001, 0b10000001, 0b10000001, 0b01000010, 0b00111100,
                       0b00111100, 0b01000010, 0b10000001, 0b10000001, 0b10000001, 0b10000001, 0b01000010, 0b00111100, 0b10000001, 0b10000001, 0b01000010, 0b00111100, 0b00111100, 0b01000010, 0b10000001, 0b10000001,
                       0b00111100, 0b01000010, 0b10000001, 0b10000001, 0b10000001, 0b10000001, 0b01000010, 0b00111100, 0b10000001, 0b10000001, 0b01000010, 0b00111100, 0b00111100, 0b01000010, 0b10000001, 0b10000001,
                       0b10000001, 0b10000001, 0b01000010, 0b00111100, 0b00111100, 0b01000010, 0b10000001, 0b10000001, 0b00111100, 0b01000010, 0b10000001, 0b10000001, 0b10000001, 0b10000001, 0b01000010, 0b00111100};

typedef union
{
    uint8_t p[2];
    uint16_t word;
} img_4x4_t;

bool reflect_h(img_4x4_t *img)
{
    uint16_t w = img->word;
    return ((w & 0x8888) >> 4 == (w & 0x1111)) && ((w & 0x4444) >> 1 == (w & 0x2222));
}

bool reflect_v(img_4x4_t *img)
{
    uint16_t w = img->word;
    return ((w & 0xF000) >> 12 == (w & 0x000F)) && ((w & 0x0F00) >> 4 == (w & 0x00F0));
}

bool reflect_d(img_4x4_t *img)
{
    uint16_t w = img->word;
    return ((w & 0x0100) >> 1 == (w & 0x0080)) && ((w & 0x0200) >> 2);
}

int main()
{
    //    uint8_t *qt = lqt_from_bitmap(test_img2, 5);
    //    lqt_print(qt);
    qt_ir_t *ir = qt_ir_create(test_img2, 5);
    qt_ir_print_tree(ir);
    qt_ir_print_img(ir);
}