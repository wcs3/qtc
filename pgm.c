#include "pgm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

enum
{
    PARSE_STEP_MAGIC0,
    PARSE_STEP_MAGIC1,
    PARSE_STEP_WIDTH,
    PARSE_STEP_HEIGHT,
    PARSE_STEP_END
};

uint8_t *pgm_decode(const uint8_t *data, uint32_t size, uint16_t *w, uint16_t *h)
{
    uint32_t i = 0;

    uint8_t parse_step = PARSE_STEP_MAGIC0;

    bool parsing_comment = false;

    while (i < size && parse_step != PARSE_STEP_END)
    {
        
    }
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

    uint32_t bytes_read = fread(data, 1, size, f);
    fclose(f);

    uint8_t *pixels = (bytes_read != size) ? NULL : pgm_decode(data, size, w, h);
    free(data);

    return pixels;
}