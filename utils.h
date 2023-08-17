#ifndef __UTILS_H__
#define __UTILS_H__

#include "types.h"

#include <stdint.h>
#include <stdbool.h>


#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MIN4(a, b, c, d) (MIN(MIN(a, b), MIN(c, d)))
#define MAX(a, b) ((a) < (b) ? (b) : (a))

/**
 * Write a value to a nibble array
 *
 * @param p pointer to nibble array
 * @param i nibble index
 * @param val 4-bit value (4 lsbs are masked out)
 */
static inline void na_write(u8 *na, u32 i, u8 val)
{
    u32 byte_index = i / 2;
    u8 nibble_shift = (i & 1) * 4;

    na[byte_index] &= ~(0xF << nibble_shift);
    na[byte_index] |= (val & 0xF) << nibble_shift;
}

/**
 * Read a value from a nibble array
 *
 * @param p pointer to nibble array
 * @param i nibble index
 * @return 4-bit value at index i
 */
static inline u8 na_read(const u8 *na, u32 i)
{
    u32 byte_index = i / 2;
    u8 nibble_shift = (i & 1) * 4;

    return (na[byte_index] >> nibble_shift) & 0xF;
}

static inline void ba_write(u8 *ba, u32 i, bool val)
{
    u32 byte_index = i / 8;
    u8 bit_shift = i % 8;

    ba[byte_index] &= ~(0x1 << bit_shift);
    ba[byte_index] |= val << bit_shift;
}

static inline bool ba_read(const u8 *ba, u32 i)
{
    u32 byte_index = i / 8;
    u32 bit_shift = i % 8;

    return (ba[byte_index] >> bit_shift) & 0x1;
}

static inline void arr_invert(u8 *arr, const u32 size)
{
    u8 *arr_end = arr + size;
    while (arr < arr_end)
    {
        (*arr) = ~(*arr);
        arr++;
    }
}

#endif // __UTILS_H__