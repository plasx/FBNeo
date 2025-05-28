#include <stdint.h>
#include <cstddef> // For NULL

// Define types that are needed before including other headers
typedef int INT32;
typedef short INT16;
typedef unsigned int UINT32;
typedef unsigned short UINT16;
typedef unsigned char UINT8;

#include "export_helpers.h"

// This cpp file exports Neo Geo functions to the Metal backend
extern "C" {
    // Neo Geo functions
    int NeoRender() { return 0; }
    int NeoPrepare() { return 0; }
    void NeoDecodeSprites(bool bGraphics) {}
    void NeoUpdateSpritesCache() {}
    void NeoRenderSprites() {}
    void NeoUpdatePalette() {}
    void NeoSetTileBank(unsigned int nOffset, unsigned char nBank) {}
    void NeoUpdatePalette32(uint32_t* pPalSrc) {}
    void NeoPalInitCallback(uint32_t* pPal) {}
    void NeoFillPalette(int color) {}
    void NeoExitSprites() {}
    int __cdecl NeoLoadSprites_1(unsigned char* Src, int nStart, int nCount) { return 0; }
    unsigned int* NeoSpriteAvailable() { return NULL; }
    void NeoInitSprites() {}
    void NeoInitTiles16() {}
    void NeoExitTiles16() {}
    void NeoUpdateTiles16(unsigned char* pGraphics, int nLen) {}
    unsigned char* NeoTileAttrib16() { return NULL; }
    int __cdecl NeoLoadTiles16(unsigned char* pSrc, int nStart, int nTiles) { return 0; }
    int __cdecl NeoLoadTiles16_Hard(unsigned char* pSrc, int nStart, int nTiles) { return 0; }
    void NeoRenderTile16(unsigned char* pDest, unsigned int* pTile, int nPalette, int nColourlevel, int nLine, int nPriority) {}
    void NeoRenderText() {}
    void NeoRenderFix(int nLayer) {}
    void NeoExitFixLayer() {}
    void NeoInitFixLayer() {}
    int __cdecl NeoLoadFix(unsigned char* pData, int nLen) { return 0; }
    int NeoRenderColourDepth() { return 0; }
    
    // Metal specific version of NeoCDInfo
    int NeoCDInfo_MVS_ID(void) { return 0; }
    int NeoCDInfo_MVS_Text(int i) { return 0; }
} 