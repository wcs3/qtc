#ifndef __BPS_H__
#define __BPS_H__

#include <stdint.h>

uint8_t *bit_plane_slice_8(const uint8_t *data, uint16_t w, uint16_t h, uint32_t *bp_size);
uint8_t *bit_plane_unslice_8(const uint8_t *bps, uint16_t w, uint16_t h);

#endif // __BPS_H__