#ifndef __FILT_UP_H__
#define __FILT_UP_H__

#include "types.h"

u8 *filt_up_apply(const u8 *data, u16 w, u16 h);

u8 *filt_up_remove(const u8 *data, u16 w, u16 h);

u8 *filt_paeth_apply(const u8 *data, u16 w, u16 h);

u8 *filt_left_apply(const u8 *data, u16 w, u16 h);

#endif // __FILT_UP_H__