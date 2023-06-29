#ifndef __MORTON_H__
#define __MORTON_H__

#include "types.h"

u32 morton_encode(u16 x, u16 y);
void morton_decode(u32 morton, u16 *x, u16 *y);

void morton_inc_x(u32 *morton);
void morton_inc_y(u32 *morton);

void morton_set_zero_x(u32 *morton);
void morton_set_zero_y(u32 *morton);

#endif // __MORTON_H__