/*
 * Copyright (c) 2007 NVIDIA, Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <miline.h>

#include "g80_type.h"
#include "g80_dma.h"
#include "g80_xaa.h"

void
G80Sync(ScrnInfoPtr pScrn)
{
    G80Ptr pNv = G80PTR(pScrn);
    volatile CARD16 *pSync = (volatile CARD16*)&pNv->reg[0x00711008/4] + 1;

    G80DmaStart(pNv, 0x104, 1);
    G80DmaNext (pNv, 0);
    G80DmaStart(pNv, 0x100, 1);
    G80DmaNext (pNv, 0);

    *pSync = 0x8000;
    G80DmaKickoff(pNv);
    while(*pSync);
}

void
G80DMAKickoffCallback(ScrnInfoPtr pScrn)
{
    G80Ptr pNv = G80PTR(pScrn);

    G80DmaKickoff(pNv);
    pNv->DMAKickoffCallback = NULL;
}

void
G80SetPattern(G80Ptr pNv, int bg, int fg, int pat0, int pat1)
{
    G80DmaStart(pNv, 0x2f0, 4);
    G80DmaNext (pNv, bg);
    G80DmaNext (pNv, fg);
    G80DmaNext (pNv, pat0);
    G80DmaNext (pNv, pat1);
}

void
G80SetRopSolid(G80Ptr pNv, CARD32 rop, CARD32 planemask)
{
    static const int rops[] = {
        0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0, 0x10, 0x90, 0x50, 0xD0,
        0x30, 0xB0, 0x70, 0xF0
    };

    if(planemask != ~0) {
        G80SetPattern(pNv, 0, planemask, ~0, ~0);
        if(pNv->currentRop != (rop + 32)) {
            pNv->currentRop = rop + 32;

            rop = rops[rop] | 0xA;
            G80DmaStart(pNv, 0x2a0, 1);
            G80DmaNext (pNv, rop);
        }
    } else if(pNv->currentRop != rop) {
        if(pNv->currentRop >= 16)
            G80SetPattern(pNv, ~0, ~0, ~0, ~0);
        pNv->currentRop = rop;

        rop = rops[rop];
        rop |= rop >> 4;
        G80DmaStart(pNv, 0x2a0, 1);
        G80DmaNext (pNv, rop);
    }
}

inline void
G80SetClip(G80Ptr pNv, int x, int y, int w, int h)
{
    G80DmaStart(pNv, 0x280, 4);
    G80DmaNext (pNv, x);
    G80DmaNext (pNv, y);
    G80DmaNext (pNv, w);
    G80DmaNext (pNv, h);
}
