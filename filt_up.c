#include "filt_up.h"

u8 *filt_up_apply(const u8 *data, u16 w, u16 h)
{
    u8 *filt = malloc(w * h);

    const u8 *data_curr = data;
    u8 *filt_curr = filt;

    for (u16 x = 0; x < w; x++)
    {
        *filt_curr = *data_curr;
        data_curr++;
        filt_curr++;
    }

    const u8 *data_prev = data;

    for (u16 y = 1; y < h; y++)
    {
        for (u16 x = 0; x < w; x++)
        {
            *filt_curr = *data_curr - *data_prev;
            data_curr++;
            data_prev++;
            filt_curr++;
        }
    }

    return filt;
}

u8 *filt_left_apply(const u8 *data, u16 w, u16 h)
{
    u8 *filt = malloc(w * h);

    const u8 *data_curr = data;
    u8 *filt_curr = filt;

    for (u16 y = 0; y < h; y++)
    {
        u8 data_prev = 0;

        for (u16 x = 0; x < w; x++)
        {
            *filt_curr = *data_curr - data_prev;
            data_prev = *data_curr;

            data_curr++;
            filt_curr++;
        }
    }

    return filt;
}

static u8 get_paeth_predictor(u8 l, u8 u, u8 lu)
{
    s16 bv = (s16)l + (s16)u - (s16)lu;

    u16 bv_l = abs((s16)l - bv);
    u16 bv_u = abs((s16)u - bv);
    u16 bv_lu = abs((s16)lu - bv);

    u8 pp;

    if (bv_u < bv_l)
    {
        if (bv_lu < bv_u)
        {
            pp = lu;
        }
        else
        {
            pp = u;
        }
    }
    else
    {
        if (bv_lu < bv_l)
        {
            pp = lu;
        }
        else
        {
            pp = l;
        }
    }

    return pp;
}

u8 *filt_paeth_apply(const u8 *data, u16 w, u16 h)
{
    u8 *filt = malloc(w * h);

    u8 u = 0;
    u8 l = 0;
    u8 lu = 0;

    u8 *filt_curr = filt;
    const u8 *data_curr = data;

    for (u16 x = 0; x < w; x++)
    {
        *filt_curr = *data_curr - get_paeth_predictor(l, u, lu);
        l = *data_curr;
        filt_curr++;
        data_curr++;
    }

    const u8 * u_p = data;

    for (u16 y = 1; y < h; y++)
    {
        l = 0;
        lu = 0;

        for (u16 x = 0; x < w; x++)
        {
            *filt_curr = *data_curr - get_paeth_predictor(l, *u_p, lu);
            lu = *u_p;
            l = *data_curr;

            u_p++;
            data_curr++;
            filt_curr++; 
        }
    }

    return filt;
}

u8 *filt_up_remove(const u8 *data, u16 w, u16 h)
{
    u8 *defilt = malloc(w * h);

    u8 *defilt_curr = defilt;
    const u8 *data_curr = data;

    for (u16 x = 0; x < w; x++)
    {
        *defilt_curr = *data_curr;
        defilt_curr++;
        data_curr++;
    }

    u8 *defilt_prev = defilt;

    for (u16 y = 1; y < h; y++)
    {
        for (u16 x = 0; x < w; x++)
        {
            *defilt_curr = *defilt_prev + *data_curr;
            defilt_curr++;
            defilt_prev++;
            data_curr++;
        }
    }

    return defilt;
}