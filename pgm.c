#include "pgm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

enum
{
    PARSE_STEP_MAGIC,
    PARSE_STEP_DIMENSIONS,
    PARSE_STEP_HEIGHT,
    PARSE_STEP_GREY_LVLS,
    PARSE_STEP_END
};

uint32_t get_line_end(const uint8_t *data, uint32_t size, uint32_t i)
{
    while (i < size && data[i] != '\n')
    {
        i++;
    }

    return i;
}

const uint8_t *get_alnum_char(const uint8_t *data_start, const uint8_t *data_end)
{
    const uint8_t *p = data_start;
    while (p < data_end)
    {
        if (isalnum(*p))
        {
            return p;
        }
        p++;
    }

    return NULL;
}

uint8_t *pgm_decode(const uint8_t *data, uint32_t size, uint16_t *w, uint16_t *h)
{
    uint32_t line_start = 0;

    uint8_t parse_step = PARSE_STEP_MAGIC;

    while (parse_step != PARSE_STEP_END)
    {
        uint32_t line_end = get_line_end(data, size, line_start);
        if (line_end >= size)
        {
            return NULL;
        }

        if (line_start != line_end && data[line_start] != '#')
        {
            uint32_t line_len = line_end - line_start + 1;
            char *line = malloc(line_len);
            memcpy(line, data + line_start, line_len);
            line[line_len - 1] = '\0';

            switch (parse_step)
            {
            case PARSE_STEP_MAGIC:
            {
                char m0 = 0;
                char m1 = 0;
                int n = sscanf(line, " %c%c", &m0, &m1);
                if (n == 2 && m0 == 'P' && m1 == '5')
                {
                    parse_step = PARSE_STEP_DIMENSIONS;
                }
                else
                {
                    return NULL;
                }
                break;
            }
            case PARSE_STEP_DIMENSIONS:
            {
                uint32_t greys = 0;
                int n = sscanf(line, " %hu %hu %u", w, h, &greys);
                if (n < 1)
                {
                    return NULL;
                }
                else if (n == 1)
                {
                    parse_step = PARSE_STEP_HEIGHT;
                }
                else if (n == 2)
                {
                    parse_step = PARSE_STEP_GREY_LVLS;
                }
                else if (n == 3)
                {
                    if (greys != 255)
                    {
                        return NULL;
                    }
                    parse_step = PARSE_STEP_END;
                }
                break;
            }
            case PARSE_STEP_HEIGHT:
            {
                uint32_t greys = 0;
                int n = sscanf(line, " %hu %u", h, &greys);
                if (n < 1)
                {
                    return NULL;
                }
                else if (n == 1)
                {
                    parse_step = PARSE_STEP_GREY_LVLS;
                }
                else if (n == 2)
                {
                    if (greys != 255)
                    {
                        return NULL;
                    }
                    parse_step = PARSE_STEP_END;
                }
                break;
            }
            case PARSE_STEP_GREY_LVLS:
            {
                uint32_t greys = 0;
                int n = sscanf(line, " %u", &greys);
                if (n < 1)
                {
                    return NULL;
                }
                else if (greys != 255)
                {
                    return NULL;
                }
                parse_step = PARSE_STEP_END;
                break;
            }
            default:
                break;
            }

            free(line);
        }

        line_start = line_end + 1;
    }

    const uint8_t *pixel_data = data + line_start;
    uint32_t px_cnt = (*w) * (*h);

    if (line_start + px_cnt > size)
    {
        return NULL;
    }

    uint8_t *pixels = malloc(px_cnt);

    if (!pixels)
    {
        return NULL;
    }

    for (uint32_t i = 0; i < px_cnt; i++)
    {
        pixels[i] = pixel_data[i];
    }

    return pixels;
}

uint8_t *pgm_read(const char *filename, uint16_t *w, uint16_t *h)
{
    FILE *f = fopen(filename, "rb");
    if (!f)
    {
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    int size = ftell(f);
    if (size <= 0 || fseek(f, 0, SEEK_SET) != 0)
    {
        fclose(f);
        return NULL;
    }

    uint8_t *data = malloc(size);
    if (!data)
    {
        fclose(f);
        return NULL;
    }

    int bytes_read = fread(data, 1, size, f);
    fclose(f);

    uint8_t *pixels = (bytes_read != size) ? NULL : pgm_decode(data, size, w, h);
    free(data);

    return pixels;
}