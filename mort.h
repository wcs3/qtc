#ifndef __MORTON_H__
#define __MORTON_H__

#include <stdint.h>

/**
 * Get morton code from x and y coordinates
 *
 * @param x x coordinate
 * @param y y coordinate
 *
 * @return morton encoding of x and y coordinates
 */
uint32_t morton_encode(uint16_t x, uint16_t y);

/**
 * Get x and y coordinates from a morton code
 *
 * @param morton morton code
 * @param x pointer to variable that will hold x coord
 * @param y pointer to variable that will hold y coord
 */
void morton_decode(uint32_t morton, uint16_t *x, uint16_t *y);

/**
 * Increment the x value within a morton encoding
 *
 * @param morton pointer to morton code
 */
void morton_inc_x(uint32_t *morton);

/**
 * Increment the y value within a morton encoding
 *
 * @param morton pointer to morton code
 */
void morton_inc_y(uint32_t *morton);

/**
 * Set the x value within a morton encoding to 0
 *
 * @param morton pointer to morton code
 */
void morton_rst_x(uint32_t *morton);

/**
 * Set the y value within a morton encoding to 0
 *
 * @param morton pointer to morton code
 */
void morton_rst_y(uint32_t *morton);

#endif // __MORTON_H__