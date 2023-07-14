#ifndef __XBN_H__
#define __XBN_H__

#include <stdint.h>

uint8_t *xbn_encode(const uint8_t *data,
                    const uint32_t size,
                    const uint8_t x,
                    uint8_t *bd_n,
                    uint32_t *out_size);

uint8_t *xbn_decode(const uint8_t *xbn,
                    const uint32_t size,
                    const uint8_t x,
                    const uint8_t bd_n);

uint8_t *xbsn_encode(const uint8_t *data,
                     const uint32_t size,
                     const uint8_t x,
                     uint8_t *bd_s,
                     uint32_t *out_size);

uint8_t *xbsn_decode(const uint8_t *xbn,
                     const uint32_t size,
                     const uint8_t x,
                     const uint8_t bd_s);

#endif // __XBN_H__