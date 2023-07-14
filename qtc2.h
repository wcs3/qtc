#ifndef __QTC2_H__
#define __QTC2_H__

#include "types.h"

void qtc2_encode(const u8 *pix, u16 w, u16 h, u8 **qtc, u32 *qtc_size);
void qtc2_decode(const u8 *qtc, u16 w, u16 h, u8 **pix, u32 *comp_size);

#endif // __QTC2_H__