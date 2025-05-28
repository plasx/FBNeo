#ifndef _CTV_DO_H_
#define _CTV_DO_H_

typedef INT32 (*CtvDoFn)();

// Alpha blending function declaration
static inline UINT32 alpha_blend(UINT32 d, UINT32 s, UINT32 p) {
    if (p == 0) return d;
    
    INT32 a = 255 - p;
    
    return (((((s & 0xff00ff) * p) + ((d & 0xff00ff) * a)) & 0xff00ff00) +
            ((((s & 0x00ff00) * p) + ((d & 0x00ff00) * a)) & 0x00ff0000)) >> 8;
}

// CPS Tiles (header)

// Clip values
#define CLIP_8 0x3F
#define CLIP_16 0x1F

// Macros for plotting pixels
#define NEXTPIXEL pPix += nBurnBpp;
#define NEXTLINE pPix += nBurnPitch - (nBurnBpp << 3);

#define PLOT { if (nCpsBlend) { c = alpha_blend(pPix[0]|(pPix[1]<<8)|(pPix[2]<<16), c, nCpsBlend); } pPix[0]=(UINT8)c; pPix[1]=(UINT8)(c>>8); pPix[2]=(UINT8)(c>>16); }

// Macros for drawing pixels
#define DRAWPIXEL { if (b & 0xf0000000) { c = ctp[b >> 28]; PLOT } }
#define DRAWPIXEL_FLIPX { if (b & 0x0000000f) { c = ctp[b & 15]; PLOT } }

// Macros for drawing eight pixels
#define DO_PIX DRAWPIXEL NEXTPIXEL
#define DO_PIX_FLIPX DRAWPIXEL_FLIPX NEXTPIXEL

#define EIGHT(DO) DO DO DO DO DO DO DO DO

#define DRAW_8 nBlank |= b; EIGHT(DO_PIX)
#define DRAW_8_FLIPX nBlank |= b; EIGHT(DO_PIX_FLIPX)

// Tile drawing functions
INT32 CtvDo208____();
INT32 CtvDo208__f_();
INT32 CtvDo208_c__();
INT32 CtvDo208_cf_();
INT32 CtvDo216____();
INT32 CtvDo216__f_();
INT32 CtvDo216_c__();
INT32 CtvDo216_cf_();
INT32 CtvDo216r___();
INT32 CtvDo216r_f_();
INT32 CtvDo216rc__();
INT32 CtvDo216rcf_();

#endif // _CTV_DO_H_
