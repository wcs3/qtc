#ifndef __QTIR_H__
#define __QTIR_H__

#include "types.h"

enum
{
    COMP_CODE_NONE = 0,

    // Subtree filled with 1111
    COMP_CODE_FILL, // |1|1|
                    // |1|1|

    // X in 3 children, 0000 in remaining
    COMP_CODE_XXX0, // |0|X|
                    // |X|X|

    COMP_CODE_XX0X, // |X|0|
                    // |X|X|

    COMP_CODE_X0XX, // |X|X|
                    // |0|X|

    COMP_CODE_0XXX, // |X|X|
                    // |X|0|

    // X in 3 children, Y in remaining
    COMP_CODE_XXXY, // |Y|X|
                    // |X|X|

    COMP_CODE_XXYX, // |X|Y|
                    // |X|X|

    COMP_CODE_XYXX, // |X|X|
                    // |Y|X|

    COMP_CODE_YXXX, // |X|X|
                    // |X|Y|

    // X in 2 children, Y in other 2
    COMP_CODE_XXYY, // |Y|Y|
                    // |X|X|

    COMP_CODE_XYYX, // |X|Y|
                    // |Y|X|

    COMP_CODE_XYXY, // |Y|X|
                    // |Y|X|

    // X in children
    COMP_CODE_XXXX, // |X|X|
                    // |X|X|

    COMP_CODE_Cnt
};

typedef struct qtir_node qtir_node;

struct qtir_node
{
    qtir_node *quads[QUAD_Cnt];
    bool compressed;
    u8 code;
};

/**
 * Convert array of pixels in byte-padded rows to quad tree intermediate representation
 */
void qtir_from_pix(const u8 *pix, u16 w, u16 h, qtir_node **ir, u8 *height);

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