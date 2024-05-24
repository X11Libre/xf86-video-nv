/*
 * Copyright (c) 2003 NVIDIA, Corporation
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

#include "nv_include.h"
#include "miline.h"
#include "nv_dma.h"

static const int NVCopyROP[16] =
{
   0x00,            /* GXclear */
   0x88,            /* GXand */
   0x44,            /* GXandReverse */
   0xCC,            /* GXcopy */
   0x22,            /* GXandInverted */
   0xAA,            /* GXnoop */
   0x66,            /* GXxor */
   0xEE,            /* GXor */
   0x11,            /* GXnor */
   0x99,            /* GXequiv */
   0x55,            /* GXinvert*/
   0xDD,            /* GXorReverse */
   0x33,            /* GXcopyInverted */
   0xBB,            /* GXorInverted */
   0x77,            /* GXnand */
   0xFF             /* GXset */
};

static const int NVCopyROP_PM[16] =
{
   0x0A,            /* GXclear */
   0x8A,            /* GXand */
   0x4A,            /* GXandReverse */
   0xCA,            /* GXcopy */
   0x2A,            /* GXandInverted */
   0xAA,            /* GXnoop */
   0x6A,            /* GXxor */
   0xEA,            /* GXor */
   0x1A,            /* GXnor */
   0x9A,            /* GXequiv */
   0x5A,            /* GXinvert*/
   0xDA,            /* GXorReverse */
   0x3A,            /* GXcopyInverted */
   0xBA,            /* GXorInverted */
   0x7A,            /* GXnand */
   0xFA             /* GXset */
};

void
NVDmaKickoff(NVPtr pNv)
{
    if(pNv->dmaCurrent != pNv->dmaPut) {
        pNv->dmaPut = pNv->dmaCurrent;
        WRITE_PUT(pNv,  pNv->dmaPut);
    }
}


/* There is a HW race condition with videoram command buffers.
   You can't jump to the location of your put offset.  We write put
   at the jump offset + SKIPS dwords with noop padding in between
   to solve this problem */
#define SKIPS  8

void 
NVDmaWait (
   NVPtr pNv,
   int size
){
    int dmaGet;

    size++;

    while(pNv->dmaFree < size) {
       dmaGet = READ_GET(pNv);

       if(pNv->dmaPut >= dmaGet) {
           pNv->dmaFree = pNv->dmaMax - pNv->dmaCurrent;
           if(pNv->dmaFree < size) {
               NVDmaNext(pNv, 0x20000000);
               if(dmaGet <= SKIPS) {
                   if(pNv->dmaPut <= SKIPS) /* corner case - will be idle */
                      WRITE_PUT(pNv, SKIPS + 1);
                   do { dmaGet = READ_GET(pNv); }
                   while(dmaGet <= SKIPS);
               }
               WRITE_PUT(pNv, SKIPS);
               pNv->dmaCurrent = pNv->dmaPut = SKIPS;
               pNv->dmaFree = dmaGet - (SKIPS + 1);
           }
       } else 
           pNv->dmaFree = dmaGet - pNv->dmaCurrent - 1;
    }
}

void
NVWaitVSync(NVPtr pNv)
{
    NVDmaStart(pNv, 0x0000A12C, 1);
    NVDmaNext (pNv, 0);
    NVDmaStart(pNv, 0x0000A134, 1);
    NVDmaNext (pNv, pNv->CRTCnumber);
    NVDmaStart(pNv, 0x0000A100, 1);
    NVDmaNext (pNv, 0);
    NVDmaStart(pNv, 0x0000A130, 1);
    NVDmaNext (pNv, 0);
}

/* 
  currentRop =  0-15  solid fill
               16-31  8x8 pattern fill
               32-47  solid fill with planemask 
*/

static void 
NVSetPattern(
   ScrnInfoPtr pScrn,
   CARD32 clr0,
   CARD32 clr1,
   CARD32 pat0,
   CARD32 pat1
)
{
    NVPtr pNv = NVPTR(pScrn);

    NVDmaStart(pNv, PATTERN_COLOR_0, 4);
    NVDmaNext (pNv, clr0);
    NVDmaNext (pNv, clr1);
    NVDmaNext (pNv, pat0);
    NVDmaNext (pNv, pat1);
}

static void 
NVSetRopSolid(ScrnInfoPtr pScrn, CARD32 rop, CARD32 planemask)
{
    NVPtr pNv = NVPTR(pScrn);

    if(planemask != ~0) {
        NVSetPattern(pScrn, 0, planemask, ~0, ~0);
        if(pNv->currentRop != (rop + 32)) {
           NVDmaStart(pNv, ROP_SET, 1);
           NVDmaNext (pNv, NVCopyROP_PM[rop]);
           pNv->currentRop = rop + 32;
        }
    } else 
    if (pNv->currentRop != rop) {
        if(pNv->currentRop >= 16)
             NVSetPattern(pScrn, ~0, ~0, ~0, ~0);
        NVDmaStart(pNv, ROP_SET, 1);
        NVDmaNext (pNv, NVCopyROP[rop]);
        pNv->currentRop = rop;
    }
}

void NVResetGraphics(ScrnInfoPtr pScrn)
{
    NVPtr pNv = NVPTR(pScrn);
    CARD32 surfaceFormat, patternFormat, rectFormat, lineFormat;
    int pitch, i;

    if(pNv->NoAccel) return;

    pitch = pNv->CurrentLayout.displayWidth * 
            (pNv->CurrentLayout.bitsPerPixel >> 3);

    pNv->dmaBase = (CARD32*)(&pNv->FbStart[pNv->FbUsableSize]);

    for(i = 0; i < SKIPS; i++)
      pNv->dmaBase[i] = 0x00000000;

    pNv->dmaBase[0x0 + SKIPS] = 0x00040000;
    pNv->dmaBase[0x1 + SKIPS] = 0x80000010;
    pNv->dmaBase[0x2 + SKIPS] = 0x00042000;
    pNv->dmaBase[0x3 + SKIPS] = 0x80000011;
    pNv->dmaBase[0x4 + SKIPS] = 0x00044000;
    pNv->dmaBase[0x5 + SKIPS] = 0x80000012;
    pNv->dmaBase[0x6 + SKIPS] = 0x00046000;
    pNv->dmaBase[0x7 + SKIPS] = 0x80000013;
    pNv->dmaBase[0x8 + SKIPS] = 0x00048000;
    pNv->dmaBase[0x9 + SKIPS] = 0x80000014;
    pNv->dmaBase[0xA + SKIPS] = 0x0004A000;
    pNv->dmaBase[0xB + SKIPS] = 0x80000015;
    pNv->dmaBase[0xC + SKIPS] = 0x0004C000;
    pNv->dmaBase[0xD + SKIPS] = 0x80000016;
    pNv->dmaBase[0xE + SKIPS] = 0x0004E000;
    pNv->dmaBase[0xF + SKIPS] = 0x80000017;

    pNv->dmaPut = 0;
    pNv->dmaCurrent = 16 + SKIPS;
    pNv->dmaMax = 8191;
    pNv->dmaFree = pNv->dmaMax - pNv->dmaCurrent;

    switch(pNv->CurrentLayout.depth) {
    case 24:
       surfaceFormat = SURFACE_FORMAT_DEPTH24;
       patternFormat = PATTERN_FORMAT_DEPTH24;
       rectFormat    = RECT_FORMAT_DEPTH24;
       lineFormat    = LINE_FORMAT_DEPTH24;
       break;
    case 16:
    case 15:
       surfaceFormat = SURFACE_FORMAT_DEPTH16;
       patternFormat = PATTERN_FORMAT_DEPTH16;
       rectFormat    = RECT_FORMAT_DEPTH16;
       lineFormat    = LINE_FORMAT_DEPTH16;
       break;
    default:
       surfaceFormat = SURFACE_FORMAT_DEPTH8;
       patternFormat = PATTERN_FORMAT_DEPTH8;
       rectFormat    = RECT_FORMAT_DEPTH8;
       lineFormat    = LINE_FORMAT_DEPTH8;
       break;
    }

    NVDmaStart(pNv, SURFACE_FORMAT, 4);
    NVDmaNext (pNv, surfaceFormat);
    NVDmaNext (pNv, pitch | (pitch << 16));
    NVDmaNext (pNv, 0);
    NVDmaNext (pNv, 0);

    NVDmaStart(pNv, PATTERN_FORMAT, 1);
    NVDmaNext (pNv, patternFormat);

    NVDmaStart(pNv, RECT_FORMAT, 1);
    NVDmaNext (pNv, rectFormat);

    NVDmaStart(pNv, LINE_FORMAT, 1);
    NVDmaNext (pNv, lineFormat);

    pNv->currentRop = ~0;  /* set to something invalid */
    NVSetRopSolid(pScrn, GXcopy, ~0);

    NVDmaKickoff(pNv);
}

void NVSync(ScrnInfoPtr pScrn)
{
    NVPtr pNv = NVPTR(pScrn);

    if(pNv->DMAKickoffCallback)
       (*pNv->DMAKickoffCallback)(pScrn);

    while(READ_GET(pNv) != pNv->dmaPut);

    while(pNv->PGRAPH[0x0700/4]);
}
