#include "mort.h"

static uint32_t interleave_zeroes_u16(uint16_t x);
static uint16_t get_even_bits_u32(uint32_t x);

uint32_t morton_encode(uint16_t x, uint16_t y)
{
    return interleave_zeroes_u16(x) | (interleave_zeroes_u16(y) << 1);
}

void morton_decode(uint32_t morton, uint16_t *x, uint16_t *y)
{
    *x = get_even_bits_u32(morton);
    *y = get_even_bits_u32(morton >> 1);
}

void morton_inc_x(uint32_t *morton)
{
    uint32_t xsum = (*morton | 0xAAAAAAAA) + 1;
    *morton = (xsum & 0x55555555) | (*morton & 0xAAAAAAAA);
}

void morton_rst_x(uint32_t *morton)
{
    *morton &= ~(0x55555555);
}

void morton_inc_y(uint32_t *morton)
{
    uint32_t ysum = (*morton | 0x55555555) + 2;
    *morton = (ysum & 0xAAAAAAAA) | (*morton & 0x55555555);
}

void morton_rst_y(uint32_t *morton)
{
    *morton &= ~(0xAAAAAAAA);
}

static uint32_t interleave_zeroes_u16(uint16_t x)
{
    uint32_t y = x;

    y = (y | (y << 8)) & 0x00FF00FF;
    y = (y | (y << 4)) & 0x0F0F0F0F;
    y = (y | (y << 2)) & 0x33333333;
    y = (y | (y << 1)) & 0x55555555;

    return y;
}

static uint16_t get_even_bits_u32(uint32_t x)
{
    x = x & 0x55555555;
    x = (x | (x >> 1)) & 0x33333333;
    x = (x | (x >> 2)) & 0x0F0F0F0F;
    x = (x | (x >> 4)) & 0x00FF00FF;
    x = (x | (x >> 8)) & 0x0000FFFF;
    return (uint16_t)x;
}