#include "xbn.h"

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static void arr_write_bit(uint8_t *arr, const uint32_t i, const bool set)
{
    uint32_t byte = i / 8;
    uint8_t shift = i % 8;

    if (set)
    {
        arr[byte] |= (1 << shift);
    }
    else
    {
        arr[byte] &= ~(1 << shift);
    }
}

static bool arr_read_bit(const uint8_t *arr, const uint32_t i)
{
    uint32_t byte = i / 8;
    uint8_t shift = i % 8;

    return arr[byte] & (1 << shift);
}

static void arr_invert(uint8_t *arr, const uint32_t size)
{
    uint8_t *arr_end = arr + size;
    while (arr < arr_end)
    {
        (*arr) = ~(*arr);
        arr++;
    }
}

typedef struct
{
    uint32_t bit_pos;
    uint32_t alloced;
    uint8_t *arr;
} bsw_t;

bsw_t *bsw_create()
{
    bsw_t *bsw = malloc(sizeof(bsw_t));
    bsw->bit_pos = 0;
    bsw->alloced = 1;
    bsw->arr = malloc(bsw->alloced);
    bsw->arr[0] = 0;

    return bsw;
}

void bsw_write(bsw_t *bsw, bool set)
{
    uint32_t byte = bsw->bit_pos / 8;
    if (byte >= bsw->alloced)
    {
        bsw->alloced *= 2;
        bsw->arr = realloc(bsw->arr, bsw->alloced);
        memset(bsw->arr + byte, 0, bsw->alloced - byte);
    }

    if (set)
    {
        uint8_t shift = bsw->bit_pos % 8;
        bsw->arr[byte] |= (1 << shift);
    }
    bsw->bit_pos++;
}

static uint32_t arr_max_run_length(const uint8_t *arr, const uint32_t size)
{
    uint32_t max_run = 0;

    uint32_t i = 0;
    bool cnt_1s = arr[0] & 0x1;
    uint32_t n = 0;
    while (i < size * 8)
    {
        if (arr_read_bit(arr, i) != cnt_1s)
        {
            if (n > max_run)
            {
                max_run = n;
            }
            cnt_1s = !cnt_1s;
            n = 0;
        }

        n++;
        i++;
    }

    if (n > max_run)
    {
        max_run = n;
    }

    return max_run;
}

uint8_t *xbn_encode(const uint8_t *data,
                    const uint32_t size,
                    const uint8_t x,
                    uint8_t *bd_n,
                    uint32_t *out_size)
{
    uint32_t max_run;
    uint32_t data_pos, n;
    uint8_t k;
    bool cnt_1s;
    uint8_t *xbn;
    bsw_t *xbn_bsw;

    xbn_bsw = bsw_create();

    max_run = arr_max_run_length(data, size);

    *bd_n = 0;

    if (max_run > x)
    {
        do
        {
            (*bd_n)++;
        } while (((1U << (*bd_n)) - 1) < (max_run - x));
    }

    data_pos = 0;

    n = 0;

    cnt_1s = arr_read_bit(data, 0);

    bsw_write(xbn_bsw, cnt_1s);

    while (data_pos < size * 8)
    {
        if (arr_read_bit(data, data_pos) != cnt_1s)
        {
            if (n > x)
            {
                for (k = 0; k < x; k++)
                {
                    bsw_write(xbn_bsw, true);
                }
                n -= x;
                for (k = 0; k < *bd_n; k++)
                {
                    bsw_write(xbn_bsw, n & (1 << k));
                }
            }
            else
            {
                for (k = 0; k < n - 1; k++)
                {
                    bsw_write(xbn_bsw, true);
                }
                bsw_write(xbn_bsw, false);
            }
            n = 0;
            cnt_1s = !cnt_1s;
        }

        n++;
        data_pos++;
    }

    if (n > x)
    {
        for (k = 0; k < x; k++)
        {
            bsw_write(xbn_bsw, true);
        }
        n -= x;
        for (k = 0; k < *bd_n; k++)
        {
            bsw_write(xbn_bsw, n & (1 << k));
        }
    }
    else
    {
        for (k = 0; k < n - 1; k++)
        {
            bsw_write(xbn_bsw, true);
        }
        bsw_write(xbn_bsw, false);
    }

    printf("%u %u\n", xbn_bsw->bit_pos, *bd_n);

    *out_size = (xbn_bsw->bit_pos + 7) / 8;
    xbn = xbn_bsw->arr;
    free(xbn_bsw);
    return realloc(xbn, *out_size);
}

uint8_t *xbn_decode(const uint8_t *xbn,
                    const uint32_t size,
                    const uint8_t x,
                    const uint8_t bd_n)
{
    uint32_t data_pos, xbn_pos, n;
    uint8_t k;
    bool write_1s, bit;
    uint8_t *data;

    data = malloc(size);

    data_pos = 0;
    xbn_pos = 0;

    write_1s = arr_read_bit(xbn, xbn_pos++);

    while (data_pos < size * 8)
    {
        n = 1;

        bit = arr_read_bit(xbn, xbn_pos++);
        while (bit && n < x)
        {
            bit = arr_read_bit(xbn, xbn_pos++);
            n++;
        }

        if (bit)
        {
            for (k = 0; k < bd_n; k++)
            {
                n += arr_read_bit(xbn, xbn_pos++) << k;
            }
        }

        for (k = 0; k < n && data_pos < size * 8; k++)
        {
            arr_write_bit(data, data_pos++, write_1s);
        }

        write_1s = !write_1s;
    }

    return data;
}

uint8_t *xbsn_encode(const uint8_t *data,
                     const uint32_t size,
                     const uint8_t x,
                     uint8_t *bd_s,
                     uint32_t *out_size)
{
    uint32_t max_run;
    uint32_t data_pos, n;
    uint8_t k, max_bd_n, s;
    bool cnt_1s;
    uint8_t *xbsn;
    bsw_t *xbsn_bsw;

    xbsn_bsw = bsw_create();

    max_run = arr_max_run_length(data, size);

    max_bd_n = 0;

    if (max_run > x)
    {
        do
        {
            max_bd_n++;
        } while (((1U << max_bd_n) - 1) < (max_run - x));
    }

    *bd_s = 0;

    while ((1U << *bd_s) - 1 < max_bd_n)
    {
        (*bd_s)++;
    }

    data_pos = 0;

    n = 0;

    cnt_1s = arr_read_bit(data, 0);

    bsw_write(xbsn_bsw, cnt_1s);

    while (data_pos < size * 8)
    {
        if (arr_read_bit(data, data_pos) != cnt_1s)
        {
            if (n > x)
            {
                for (k = 0; k < x; k++)
                {
                    bsw_write(xbsn_bsw, true);
                }
                n -= x;

                s = 1;

                while ((1U << s) - 1 < n)
                {
                    s++;
                }

                for (k = 0; k < *bd_s; k++)
                {
                    bsw_write(xbsn_bsw, s & (1 << k));
                }

                for (k = 0; k < s; k++)
                {
                    bsw_write(xbsn_bsw, n & (1 << k));
                }
            }
            else
            {
                for (k = 0; k < n - 1; k++)
                {
                    bsw_write(xbsn_bsw, true);
                }
                bsw_write(xbsn_bsw, false);
            }
            n = 0;
            cnt_1s = !cnt_1s;
        }

        n++;
        data_pos++;
    }

    if (n > x)
    {
        for (k = 0; k < x; k++)
        {
            bsw_write(xbsn_bsw, true);
        }
        n -= x;

        s = 1;

        while ((1U << s) - 1 < n)
        {
            s++;
        }

        for (k = 0; k < *bd_s; k++)
        {
            bsw_write(xbsn_bsw, s & (1 << k));
        }

        for (k = 0; k < s; k++)
        {
            bsw_write(xbsn_bsw, n & (1 << k));
        }
    }
    else
    {
        for (k = 0; k < n - 1; k++)
        {
            bsw_write(xbsn_bsw, true);
        }
        bsw_write(xbsn_bsw, false);
    }

    printf("%u %u\n", xbsn_bsw->bit_pos, *bd_s);

    *out_size = (xbsn_bsw->bit_pos + 7) / 8;
    xbsn = xbsn_bsw->arr;
    free(xbsn_bsw);
    return realloc(xbsn, *out_size);
}

uint8_t *xbsn_decode(const uint8_t *xbn,
                     const uint32_t size,
                     const uint8_t x,
                     const uint8_t bd_s)
{
    uint32_t data_pos, xbn_pos, n, s;
    uint8_t k;
    bool write_1s, bit;
    uint8_t *data;

    data = malloc(size);

    data_pos = 0;
    xbn_pos = 0;

    write_1s = arr_read_bit(xbn, xbn_pos++);

    while (data_pos < size * 8)
    {
        n = 1;

        bit = arr_read_bit(xbn, xbn_pos++);

        while (bit && n < x)
        {
            bit = arr_read_bit(xbn, xbn_pos++);
            n++;
        }

        if (bit)
        {
            s = 0;

            for (k = 0; k < bd_s; k++)
            {
                s += arr_read_bit(xbn, xbn_pos++) << k;
            }

            for (k = 0; k < s; k++)
            {
                n += arr_read_bit(xbn, xbn_pos++) << k;
            }
        }

        for (k = 0; k < n; k++)
        {
            arr_write_bit(data, data_pos++, write_1s);
        }

        write_1s = !write_1s;
    }

    return data;
}