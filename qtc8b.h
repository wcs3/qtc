#ifndef __QTC8B_H__
#define __QTC8B_H__

#include "types.h"

u8 *qtc8b_encode(const u8 *data, u16 w, u16 h, u32 *out_size);

#endif // __QTC8B_H__