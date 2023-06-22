#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

enum
{
    ARG_FILENAME = 1,
    ARG_WIDTH,
    ARG_HEIGHT
};

typedef struct
{
    size_t alloced;
    size_t index;
    uint8_t bit_pos;
    uint8_t data[];
} bitstream_writer_t;

bitstream_writer_t *bitstream_writer_create()
{
    bitstream_writer_t *bsw = malloc(sizeof(bitstream_writer_t) + sizeof(uint8_t));
    bsw->data[0] = 0;
    bsw->alloced = 1;
    bsw->index = 0;
    bsw->bit_pos = 7;

    return bsw;
}

uint8_t hex_to_nibble(char hex_char) {
    if(hex_char <= '9' && hex_char >= '0') {
        return hex_char - '0';
    }

    if(hex_char <= 'F' && hex_char >= 'A') {
        return hex_char - 'A';
    } 

    if(hex_char <= 'f' && hex_char >= 'a') {
        return hex_char - 'a';
    }

    return 0;
}

void bitstream_write_bit(bitstream_writer_t **bsw, uint32_t bit)
{
    (*bsw)->data[(*bsw)->index] |= (bit & 1) << (*bsw)->bit_pos;

    if ((*bsw)->bit_pos == 0)
    {
        (*bsw)->index++;
        (*bsw)->bit_pos = 7;
        if ((*bsw)->index == (*bsw)->alloced)
        {
            (*bsw)->alloced *= 2;
            *bsw = realloc(*bsw, sizeof(bitstream_writer_t) + (*bsw)->alloced * sizeof(uint8_t));
            memset(&(*bsw)->data[(*bsw)->index], 0, (*bsw)->alloced / 2);
        }
    }
    else
    {
        (*bsw)->bit_pos--;
    }
}

void bitstream_write_bits(bitstream_writer_t **bsw, uint32_t bits, uint8_t bit_cnt)
{
    assert(bit_cnt <= 32);

    while (bit_cnt)
    {
        bitstream_write_bit(bsw, bits >> (bit_cnt - 1));
        bit_cnt--;
    }
}

int main(int argc, char *argv[])
{
    if (argc < 4)
    {
        return 1;
    }

    FILE *bmp = fopen(argv[ARG_FILENAME], "r");
    uint64_t width = strtol(argv[ARG_WIDTH], NULL, 10);
    uint64_t height = strtol(argv[ARG_HEIGHT], NULL, 10);

    uint64_t byte_w = (width + 7) / 8;

    FILE *out = fopen("out", "w");

    for (;;)
    {
    }
}