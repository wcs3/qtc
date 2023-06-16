#ifndef __VLA_H__
#define __VLA_H__

#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>

typedef struct
{
    size_t alloced;
    size_t size;
    uint8_t arr[];
} vla_t;

static inline vla_t *vla_create()
{
    vla_t *vla = (vla_t *)malloc(sizeof(vla_t));

    assert(vla);

    vla->alloced = 0;
    vla->size = 0;

    return vla;
}

static inline void vla_push(vla_t **vla, uint8_t val)
{
    assert(*vla != NULL);


    if ((*vla)->size == (*vla)->alloced)
    {
        (*vla)->alloced += (*vla)->alloced > 0 ? (*vla)->alloced : 1;
        *vla = (vla_t *)realloc(*vla, sizeof(vla_t) + (*vla)->alloced * sizeof(uint8_t));
        memset(&(*vla)->arr[(*vla)->alloced / 2], 0, (*vla)->alloced / 2);
    }

    (*vla)->arr[(*vla)->size] = val;
    (*vla)->size++;
}

static inline uint8_t vla_pop(vla_t **vla)
{
    assert(*vla != NULL);
    assert((*vla)->size > 0);

    (*vla)->size--;

    if ((*vla)->size < (*vla)->alloced / 2)
    {
        (*vla)->alloced = (*vla)->alloced / 2;
        *vla = (vla_t *)realloc(*vla, sizeof(vla_t) + (*vla)->alloced * sizeof(uint8_t));
    }

    return (*vla)->arr[(*vla)->size];
}

static inline uint8_t vla_get(vla_t const *const vla, size_t i)
{
    assert(i < vla->size);

    return vla->arr[i];
}

static inline void vla_set(vla_t *vla, size_t i, uint8_t val)
{
    assert(i < vla->size);

    vla->arr[i] = val;
}

static inline void vla_delete(vla_t *vla)
{
    free(vla);
}

#endif // __VLA_H__