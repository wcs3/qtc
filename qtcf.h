#ifndef __QTCF_H__
#define __QTCF_H__

#include "types.h"

/**
 * Compress a 1-bit raster image.
 *
 * @param data pointer to raster image data in row-major order. Rows should be byte-aligned
 * @param w image width
 * @param h image height
 * @param out_size size, in bytes, of compressed data
 *
 * @return pointer to compressed data. NULL if unsuccessful
 */
u8 *qtcf_encode(const u8 *data, u16 w, u32 h, u32 *out_size);

/**
 * Decompress a 1-bit raster image.
 *
 * @param data pointer to compressed data
 * @param w image width
 * @param h image height
 * @param in_size the number of bytes processed in the compressed data
 * 
 * @return pointer to decompressed, 1-bit raster image. NULL if unsuccessful
 */
u8 *qtcf_decode(const u8 *data, u16 w, u16 h, u32 *in_size);

#endif // __QTCF_H__