#ifndef __MORTON_H__
#define __MORTON_H__

#include <stdint.h>

uint32_t morton_from_xy(uint16_t x, uint16_t y);
void morton_to_xy(uint32_t morton, uint16_t *x, uint16_t *y);

void morton_inc_x(uint32_t *morton);
void morton_inc_y(uint32_t *morton);

void morton_set_zero_x(uint32_t *morton);
void morton_set_zero_y(uint32_t *morton);

#endif // __MORTON_H__