#ifndef __BITSTREAM_H__
#define __BITSTREAM_H__

#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
    size_t alloced;
    size_t index;
    uint8_t bit_pos;
    uint8_t data[];
} bitstream_writer_t;

typedef struct
{
    size_t write_index;
    uint8_t write_bit_pos;

    size_t read_index;
    uint8_t read_bit_pos;

    size_t alloced;
    uint8_t data[];
} bitstream_t;

static inline bitstream_t *bitstream_create()
{
    bitstream_t *bs = (bitstream_t *)malloc(sizeof(bitstream_t) + sizeof(uint8_t));

    bs->data[0] = 0;
    bs->alloced = 1;

    bs->read_index = 0;
    bs->read_bit_pos = 0;

    bs->write_index = 0;
    bs->write_bit_pos = 0;
}

static inline void bitstream_set_read_pos(bitstream_t *bs, size_t byte_index, uint8_t bit_pos)
{
    assert(byte_index < bs->write_index || bit_pos < bs->write_bit_pos);

    bs->read_index = byte_index;
    bs->read_bit_pos = bit_pos;
}

static inline uint8_t bitstream_read_bit(bitstream_t *bs)
{
    assert(bs->read_index < bs->write_index || bs->read_bit_pos < bs->write_bit_pos);

    uint8_t bit = (bs->data[bs->read_index] >> bs->read_bit_pos) & 0x1;

    bs->read_bit_pos++;
    if (bs->read_bit_pos > 7)
    {
        bs->read_bit_pos = 0;
        bs->read_index++;
    }

    return bit;
}

static inline uint32_t bitstream_read_bits(bitstream_t *bs, uint8_t bit_cnt)
{
    assert(bit_cnt <= 32);

    uint32_t bits = 0;

    for (uint8_t bit_pos = 0; bit_pos < bit_cnt; bit_pos++)
    {
        bits |= bitstream_read_bit(bs) << bit_pos;
    }

    return bits;
}

static inline void bitstream_set_write_pos(bitstream_t *bs, size_t byte_index, uint8_t bit_pos)
{
    assert(byte_index < bs->alloced && bit_pos < 8);

    bs->write_index = byte_index;
    bs->write_bit_pos = bit_pos;
}

static inline void bitstream_write_bit(bitstream_t **bs, uint8_t bit)
{
    (*bs)->data[(*bs)->write_index] |= (bit & 0x1) << (*bs)->write_bit_pos;

    if ((*bs)->write_bit_pos == 7)
    {
        (*bs)->write_index++;
        (*bs)->write_bit_pos = 0;
        if ((*bs)->write_index == (*bs)->alloced)
        {
            (*bs)->alloced *= 2;
            *bs = (bitstream_t *)realloc(*bs, sizeof(bitstream_t) + (*bs)->alloced * sizeof(uint8_t));
            memset(&(*bs)->data[(*bs)->write_index], 0, (*bs)->alloced / 2);
        }
    }
    else
    {
        (*bs)->write_bit_pos++;
    }
}

static inline void bitstream_write_bits(bitstream_t **bs, uint8_t bits, uint8_t bit_cnt)
{
    assert(bit_cnt <= 8);

    for (uint8_t bit_pos = 0; bit_pos < bit_cnt; bit_pos++)
    {
        bitstream_write_bit(bs, (bits >> bit_pos) & 0x1);
    }
}

static inline void bitstream_write_array(bitstream_t **bs, uint8_t *arr, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        bitstream_write_bits(bs, arr[i], 8);
    }
}

#endif // __BITSTREAM_H__