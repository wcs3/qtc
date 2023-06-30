#ifndef __QTC_H__
#define __QTC_H__

#include "types.h"

void qtc_encode(const u8 *pix, u16 w, u16 h, u8 **qtc, u32 *qtc_size);
void qtc_decode(u8 *qtc, u16 w, u16 h, u8 **pix);

#endif // __QTC_H__