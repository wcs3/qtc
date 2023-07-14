#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdint.h>
#include <stdbool.h>

#define NIBBLE_TO_BINARY_PATTERN "%c%c%c%c"
#define NIBBLE_TO_BINARY(nibble)     \
    ((nibble)&0x08 ? '1' : '0'),     \
        ((nibble)&0x04 ? '1' : '0'), \
        ((nibble)&0x02 ? '1' : '0'), \
        ((nibble)&0x01 ? '1' : '0')

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)       \
    ((byte)&0x80 ? '1' : '0'),     \
        ((byte)&0x40 ? '1' : '0'), \
        ((byte)&0x20 ? '1' : '0'), \
        ((byte)&0x10 ? '1' : '0'), \
        ((byte)&0x08 ? '1' : '0'), \
        ((byte)&0x04 ? '1' : '0'), \
        ((byte)&0x02 ? '1' : '0'), \
        ((byte)&0x01 ? '1' : '0')

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MIN4(a, b, c, d) (MIN(MIN(a, b), MIN(c, d)))
#define MAX(a, b) ((a) < (b) ? (b) : (a))

/**
 * Write a value to a nibble array
 *
 * @param p pointer to nibble array
 * @param i nibble index
 * @param val 4 bit value (4 lsbs are masked out)
 */
static inline void na_write(uint8_t *na, uint32_t i, uint8_t val)
{
    uint32_t byte_index = i / 2;
    uint8_t nibble_shift = (i & 1) * 4;

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
static inline uint8_t na_read(const uint8_t *na, uint32_t i)
{
    uint32_t byte_index = i / 2;
    uint8_t nibble_shift = (i & 1) * 4;

    return (na[byte_index] >> nibble_shift) & 0xF;
}

/**
 * Check if a bit is set in an array
 *
 * @param p pointer to array
 * @param i position of bit. Bit positions are as follows
 * @return true if set, false if not
 */
static inline bool bit_is_set(const uint8_t *p, uint16_t i)
{
    uint16_t byte_i = i / 8;
    uint8_t shift = i % 8;
    return (p[byte_i] >> shift) & 0x1;
}

#endif // __UTILS_H__