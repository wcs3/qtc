#ifndef __PGM_H__
#define __PGM_H__

#include <stdint.h>

uint8_t *pgm_read(const char *filename, uint16_t *w, uint16_t *h);

#endif // __PGM_H__