#include "morton.h"

static uint32_t interleave_zeroes_u16(uint16_t u16);
static uint16_t get_even_bits_u32(uint32_t u32);

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

void morton_set_zero_x(uint32_t *morton)
{
    *morton &= ~(0x55555555);
}

void morton_inc_y(uint32_t *morton)
{
    uint32_t ysum = (*morton | 0x55555555) + 2;
    *morton = (ysum & 0xAAAAAAAA) | (*morton & 0x55555555);
}

void morton_set_zero_y(uint32_t *morton)
{
    *morton &= ~(0xAAAAAAAA);
}

static uint32_t interleave_zeroes_u16(uint16_t u16)
{
    uint32_t u32 = u16;

    u32 = (u32 | (u32 << 8)) & 0b00000000111111110000000011111111;
    u32 = (u32 | (u32 << 4)) & 0b00001111000011110000111100001111;
    u32 = (u32 | (u32 << 2)) & 0b00110011001100110011001100110011;
    u32 = (u32 | (u32 << 1)) & 0b01010101010101010101010101010101;

    return u32;
}

static uint16_t get_even_bits_u32(uint32_t u32)
{
    u32 = u32 & 0x55555555;
    u32 = (u32 | (u32 >> 1)) & 0x33333333;
    u32 = (u32 | (u32 >> 2)) & 0x0F0F0F0F;
    u32 = (u32 | (u32 >> 4)) & 0x00FF00FF;
    u32 = (u32 | (u32 >> 8)) & 0x0000FFFF;
    return (uint16_t)u32;
}