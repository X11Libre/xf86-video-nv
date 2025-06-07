/*
 * Copyright (c) 1993-1999 NVIDIA, Corporation
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

/* Hacked together from mga driver and 3.3.4 NVIDIA driver by
   Jarno Paananen <jpaana@s2.org> */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "riva_include.h"
#include "xaarop.h"
#include "miline.h"

static void
RivaSetClippingRectangle(ScrnInfoPtr pScrn, int x1, int y1, int x2, int y2)
{
    int height = y2-y1 + 1;
    int width  = x2-x1 + 1;
    RivaPtr pRiva = RivaPTR(pScrn);

    RIVA_FIFO_FREE(pRiva->riva, Clip, 2);
    pRiva->riva.Clip->TopLeft     = (y1     << 16) | (x1 & 0xffff);
    pRiva->riva.Clip->WidthHeight = (height << 16) | width;
}


static void
RivaDisableClipping(ScrnInfoPtr pScrn)
{
    RivaSetClippingRectangle(pScrn, 0, 0, 0x7fff, 0x7fff);
}

/*
 * Set pattern. Internal routine. The upper bits of the colors
 * are the ALPHA bits.  0 == transparency.
 */
static void
RivaSetPattern(RivaPtr pRiva, int clr0, int clr1, int pat0, int pat1)
{
    RIVA_FIFO_FREE(pRiva->riva, Patt, 4);
    pRiva->riva.Patt->Color0        = clr0;
    pRiva->riva.Patt->Color1        = clr1;
    pRiva->riva.Patt->Monochrome[0] = pat0;
    pRiva->riva.Patt->Monochrome[1] = pat1;
}

/*
 * Set ROP.  Translate X rop into ROP3.  Internal routine.
 */
static void
RivaSetRopSolid(RivaPtr pRiva, int rop)
{    
    if (pRiva->currentRop != rop) {
        if (pRiva->currentRop >= 16)
            RivaSetPattern(pRiva, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF);
        pRiva->currentRop = rop;
        RIVA_FIFO_FREE(pRiva->riva, Rop, 1);
    }
}

void
RivaResetGraphics(ScrnInfoPtr pScrn)
{
    RivaPtr pRiva = RivaPTR(pScrn);

    if(pRiva->NoAccel) return;

    RIVA_FIFO_FREE(pRiva->riva, Patt, 1);
    pRiva->riva.Patt->Shape = 0; 
    RivaDisableClipping(pScrn);
    pRiva->currentRop = 16;  /* to force RivaSetRopSolid to reset the pattern */
    RivaSetRopSolid(pRiva, GXcopy);
}



/*
 * Synchronise with graphics engine.  Make sure it is idle before returning.
 * Should attempt to yield CPU if busy for awhile.
 */
void RivaSync(ScrnInfoPtr pScrn)
{
    RivaPtr pRiva = RivaPTR(pScrn);
    RIVA_BUSY(pRiva->riva);
}
