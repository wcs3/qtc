#ifndef __PGM_H__
#define __PGM_H__

#include <stdint.h>

/**
 * Read a pgm image and put pixel content into memory
 * 
 * @param filename name of the pgm file to read
 * @param w variable that will get the image width
 * @param h variable that will get the image height
 * 
 * @return pointer to pixel content. NULL if read unsuccessful. 
*/
uint8_t *pgm_read(const char *filename, uint16_t *w, uint16_t *h);

#endif // __PGM_H__