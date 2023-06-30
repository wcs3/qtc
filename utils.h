#ifndef __UTILS_H__
#define __UTILS_H__

#include "types.h"

/**
 * Write a value to a nibble array
 *
 * @param p pointer to nibble array
 * @param i nibble index
 * @param val 4 bit value (4 lsbs are masked out)
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
 * @return 4 bit value at index i
 */
static inline u8 na_read(u8 *na, u32 i)
{
    u32 byte_index = i / 2;
    u8 nibble_shift = (i & 1) * 4;

    return (na[byte_index] >> nibble_shift) & 0xF;
}

/**
 * Check if a bit is set in an array
 * 
 * @param p pointer to array
 * @param i position of bit. Bit positions are as follows
 * @return true if set, false if not
*/
static inline bool bit_is_set(const u8 *p, u16 i)
{
    u16 byte_i = i / 8;
    u8 shift = i % 8;
    return (p[byte_i] >> shift) & 0x1;
}

#endif // __UTILS_H__