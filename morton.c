#include "morton.h"

static u32 interleave_zeroes_u16(u16 x);
static u16 get_even_bits_u32(u32 x);

u32 morton_encode(u16 x, u16 y)
{
    return interleave_zeroes_u16(x) | (interleave_zeroes_u16(y) << 1);
}

void morton_decode(u32 morton, u16 *x, u16 *y)
{
    *x = get_even_bits_u32(morton);
    *y = get_even_bits_u32(morton >> 1);
}

void morton_inc_x(u32 *morton)
{
    u32 xsum = (*morton | 0xAAAAAAAA) + 1;
    *morton = (xsum & 0x55555555) | (*morton & 0xAAAAAAAA);
}

void morton_set_zero_x(u32 *morton)
{
    *morton &= ~(0x55555555);
}

void morton_inc_y(u32 *morton)
{
    u32 ysum = (*morton | 0x55555555) + 2;
    *morton = (ysum & 0xAAAAAAAA) | (*morton & 0x55555555);
}

void morton_set_zero_y(u32 *morton)
{
    *morton &= ~(0xAAAAAAAA);
}

static u32 interleave_zeroes_u16(u16 x)
{
    u32 y = x;

    y = (y | (y << 8)) & 0b00000000111111110000000011111111;
    y = (y | (y << 4)) & 0b00001111000011110000111100001111;
    y = (y | (y << 2)) & 0b00110011001100110011001100110011;
    y = (y | (y << 1)) & 0b01010101010101010101010101010101;

    return y;
}

static u16 get_even_bits_u32(u32 x)
{
    x = x & 0b01010101010101010101010101010101;
    x = (x | (x >> 1)) & 0b00110011001100110011001100110011;
    x = (x | (x >> 2)) & 0b00001111000011110000111100001111;
    x = (x | (x >> 4)) & 0b00000000111111110000000011111111;
    x = (x | (x >> 8)) & 0b00000000000000001111111111111111;
    return (u16)x;
}