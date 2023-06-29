#ifndef __QTIR_H__
#define __QTIR_H__

#include "types.h"

enum
{
    COMP_CODE_NONE = 0b0000,

    COMP_CODE_FILL = 0b1111,

    // X  in 3 children, 0000 in remaining
    COMP_CODE_XXX0 = 0b1110,
    COMP_CODE_XX0X = 0b1101,
    COMP_CODE_X0XX = 0b1011,
    COMP_CODE_0XXX = 0b0111,

    // X in 3 children, Y in remaining
    COMP_CODE_XXXY = 0b0001,
    COMP_CODE_XXYX = 0b0010,
    COMP_CODE_XYXX = 0b0100,
    COMP_CODE_YXXX = 0b1000,

    // X in 2 children, Y in other 2
    COMP_CODE_XXYY = 0b0011,
    COMP_CODE_XYYX = 0b0110,
    COMP_CODE_XYXY = 0b0101,

    // X in all children
    COMP_CODE_XXXX = 0b1111,
};

typedef struct qtir_node qtir_node;

struct qtir_node
{
    qtir_node *quads[QUAD_Cnt];
    u8 code;
};

/**
 * Convert array of pixels in byte-padded rows to quad tree intermediate representation
 */
void qtir_from_pix(u8 *pix, u16 w, u16 h, qtir_node **ir, u8 *height);

/**
 * Insert compression codes into intermediate representation
 */
void qtir_compress(qtir_node *ir, u8 r);

/**
 * Delete an intermediate representation tree
 */
void qtir_delete(qtir_node *node);

/**
 * Convert from a pointer based tree to a linear tree
 */
void qtir_linearize(qtir_node *ir, u8 r, u8 **qtl, u32 *qtl_n);

#endif // __QTIR_H__