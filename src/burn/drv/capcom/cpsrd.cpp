#include "burnint.h"
#include "cps.h"
#include "cps_stub.h"
#include "cpst.h"

// CPS Scroll2 with Row scroll - Draw
static INT32 nKnowBlank=-1;	// The tile we know is blank
static INT32 nFirstY, nLastY;
static INT32 bVCare;

static INT32 nXTiles = 0;
static INT32 nYTiles = 0;

inline static UINT16 *FindTile(INT32 fx,INT32 fy)
{
  INT32 p; UINT16 *pst;
  // Find tile address
  p=((fy&0x30)<<8) | ((fx&0x3f)<<6) | ((fy&0x0f)<<2);
  pst=(UINT16 *)(CpsrBase + p);
  return pst;
}

// Draw a tile line without Row Shift
static void Cps1TileLine(INT32 y,INT32 sx)
{
  INT32 x,ix,iy,sy;

  bVCare=0;
  if (y<0 || y>=nYTiles-1) bVCare=1; // Take care on the edges

  ix=(sx>>4)+1; sx&=15; sx=16-sx;
  sy=16-(nCpsrScrY&15); iy=(nCpsrScrY>>4)+1;
  nCpstY=sy+(y<<4);

  for (x=-1; x<nXTiles; x++)
  {
    UINT16 *pst; INT32 t,a;
    // Don't need to clip except around the border
    if (bVCare || x<0 || x>=nXTiles-1) nCpstType=CTT_16X16 | CTT_CARE;
    else nCpstType=CTT_16X16;

    pst=FindTile(ix+x,iy+y);
    t = BURN_ENDIAN_SWAP_INT16(pst[0]);
    
    if (Scroll2TileMask) t &= Scroll2TileMask;

   t = GfxRomBankMapper(GFXTYPE_SCROLL2, t);
   if (t == -1) continue;
   
    t<<=7; // Get real tile address
    t+=nCpsGfxScroll[2]; // add on offset to scroll tile
    if (t==nKnowBlank) continue; // Don't draw: we know it's blank
    
    a = BURN_ENDIAN_SWAP_INT16(pst[1]);

    CpstSetPal(0x40 | (a&0x1f));
    nCpstX=sx+(x<<4); nCpstTile=t; nCpstFlip=(a>>5)&3;

	if(nBgHi) CpstPmsk = BURN_ENDIAN_SWAP_INT16(*(UINT16 *)((uint8_t*)CpsSaveReg[0] + (uint8_t*)MaskAddr[(a&0x180)>>7]));
    if(CpstOneDoX[nBgHi]()) nKnowBlank=t;
  }
}

static void Cps2TileLine(INT32 y,INT32 sx)
{
  INT32 x,ix,iy,sy;

  ix=(sx>>4)+1; sx&=15; sx=16-sx;
  sy=16-(nCpsrScrY&15); iy=(nCpsrScrY>>4)+1;
  nCpstY=sy+(y<<4);

  for (x=-1; x<nXTiles; x++)
  {
    UINT16 *pst; INT32 t,a;
    // Don't need to clip except around the border
    if (bVCare || x<0 || x>=nXTiles-1) nCpstType=CTT_16X16 | CTT_CARE;
    else nCpstType=CTT_16X16;

    pst=FindTile(ix+x,iy+y);
    t = BURN_ENDIAN_SWAP_INT16(pst[0]);
    t<<=7; // Get real tile address
    t+=nCpsGfxScroll[2]; // add on offset to scroll tiles
	if (t==nKnowBlank) continue; // Don't draw: we know it's blank
    a = BURN_ENDIAN_SWAP_INT16(pst[1]);

    CpstSetPal(0x40 | (a&0x1f));
    nCpstX=sx+(x<<4); nCpstTile=t; nCpstFlip=(a>>5)&3;
    if(CpstOneDoX[2]()) nKnowBlank=t;
  }
}

// Draw a tile line with Row Shift
static void Cps1TileLineRows(INT32 y,struct CpsrLineInfo *pli)
{
  INT32 sy,iy,x;
  INT32 nTileCount;
  INT32 nLimLeft,nLimRight;

  bVCare=0;
  if (y<0 || y>=nYTiles-1) bVCare=1; // Take care on the edges

  nTileCount=pli->nTileEnd-pli->nTileStart;

  sy=16-(nCpsrScrY&15); iy=(nCpsrScrY>>4)+1;
  nCpstY=sy+(y<<4);
  CpstRowShift=pli->Rows;

  // If these rowshift limits go off the edges, we should take
  // care drawing the tile.
  nLimLeft =pli->nMaxLeft;
  nLimRight=pli->nMaxRight;
  for (x=0; x<nTileCount; x++,
    nLimLeft+=16, nLimRight+=16)
  {
    UINT16 *pst; INT32 t,a; INT32 tx; INT32 bCare;
    tx=pli->nTileStart+x;

    // See if we have to clip vertically anyway
    bCare=bVCare;
    if (bCare==0) // If we don't...
    {
      // Check screen limits of this tile
      if (nLimLeft <      0) bCare=1; // Will cross left egde
      if (nLimRight> nCpsScreenWidth-16) bCare=1; // Will cross right edge
    }
    if (bCare) nCpstType=CTT_16X16 | CTT_ROWS | CTT_CARE;
    else       nCpstType=CTT_16X16 | CTT_ROWS;

    pst=FindTile(tx,iy+y);
    t = BURN_ENDIAN_SWAP_INT16(pst[0]);
    
    if (Scroll2TileMask) t &= Scroll2TileMask;

    t = GfxRomBankMapper(GFXTYPE_SCROLL2, t);
    if (t == -1) continue;
    
    t<<=7; // Get real tile address
    t+=nCpsGfxScroll[2]; // add on offset to scroll tiles
    if (t==nKnowBlank) continue; // Don't draw: we know it's blank

    a = BURN_ENDIAN_SWAP_INT16(pst[1]);

    CpstSetPal(0x40 | (a&0x1f));

    nCpstX=x<<4; nCpstTile=t; nCpstFlip=(a>>5)&3;

	if (nBgHi) {
		CpstPmsk = BURN_ENDIAN_SWAP_INT16(*(UINT16 *)((uint8_t*)CpsSaveReg[0] + (uint8_t*)MaskAddr[(a & 0x180) >> 7]));
	}

	if(CpstOneDoX[nBgHi]()) nKnowBlank=t;
  }
}

static void Cps2TileLineRows(INT32 y,struct CpsrLineInfo *pli)
{
  INT32 sy,iy,x;
  INT32 nTileCount;
  INT32 nLimLeft,nLimRight;

  nTileCount=pli->nTileEnd-pli->nTileStart;

  sy=16-(nCpsrScrY&15); iy=(nCpsrScrY>>4)+1;
  nCpstY=sy+(y<<4);
  CpstRowShift=pli->Rows;

  // If these rowshift limits go off the edges, we should take
  // care drawing the tile.
  nLimLeft =pli->nMaxLeft;
  nLimRight=pli->nMaxRight;
  for (x=0; x<nTileCount; x++,
    nLimLeft+=16, nLimRight+=16)
  {
    UINT16 *pst; INT32 t,a; INT32 tx; INT32 bCare;
    tx=pli->nTileStart+x;

    // See if we have to clip vertically anyway
    bCare=bVCare;
    if (bCare==0) // If we don't...
    {
      // Check screen limits of this tile
      if (nLimLeft <      0) bCare=1; // Will cross left egde
      if (nLimRight> nCpsScreenWidth-16) bCare=1; // Will cross right edge
    }
    if (bCare) nCpstType=CTT_16X16 | CTT_ROWS | CTT_CARE;
    else       nCpstType=CTT_16X16 | CTT_ROWS;

    pst=FindTile(tx,iy+y);
    t = BURN_ENDIAN_SWAP_INT16(pst[0]);
    t<<=7; // Get real tile address
    t+=nCpsGfxScroll[2]; // add on offset to scroll tiles

    if (t==nKnowBlank) continue; // Don't draw: we know it's blank
    a = BURN_ENDIAN_SWAP_INT16(pst[1]);

    CpstSetPal(0x40 | (a&0x1f));

    nCpstX=x<<4; nCpstTile=t; nCpstFlip=(a>>5)&3;
    if(CpstOneDoX[2]()) nKnowBlank=t;
  }
}

INT32 Cps1rRender()
{
  INT32 y; struct CpsrLineInfo *pli;
  if (CpsrBase==NULL) return 1;
  nXTiles = nCpsScreenWidth>>4; // 16x16 tiles
  nYTiles = nCpsScreenHeight>>4;

  nKnowBlank=-1; // We don't know which tile is blank yet

  for (y=-1,pli=CpsrLineInfo; y<nYTiles; y++,pli++)
  {
    if (pli->nWidth==0)
	  Cps1TileLine(y,pli->nStart); // no rowscroll needed
    else
      Cps1TileLineRows(y,pli); // row scroll
  }
  return 0;
}

INT32 Cps2rRender()
{
	INT32 y;
	struct CpsrLineInfo *pli;
	if (CpsrBase==NULL) return 1;
	nXTiles = nCpsScreenWidth>>4; // 16x16 tiles
	nYTiles = nCpsScreenHeight>>4;

	nKnowBlank = -1;					// We don't know which tile is blank yet

	nLastY = (nEndline + (nCpsrScrY & 15)) >> 4;
	nFirstY = (nStartline + (nCpsrScrY & 15)) >> 4;
	for (y = nFirstY - 1, pli = CpsrLineInfo + nFirstY; y < nLastY; y++, pli++) {

		bVCare = ((y << 4) < nStartline) | (((y << 4) + 16) >= nEndline);

		if (pli->nWidth==0) {
			Cps2TileLine(y,pli->nStart);	// no rowscroll needed
		} else {
			Cps2TileLineRows(y,pli);		// row scroll
		}
	}
	return 0;
}

// Render a line for Scroll 2 (CPSB or CPSB-like)
static UINT8* ps;

// Render a tile on the screen
static INT32 RenderTile(UINT16* pTile, INT32 nTileXPos, INT32 nTileYPos)
{
  INT32 y;
  INT32 nX;
  INT32 nY;
  UINT16* pst;

  // Safety checks
  nX = nTileXPos; nY = nTileYPos;
  if (nX < 0 || nX > (400-16)) return 1;
  if (nY < 0 || nY > (224-16)) return 1;

  pst=(UINT16 *)(CpsrBase + ps);

  for (y=0; y<16; y++) {
    pTile[y + nY * 416 + nX]=pst[y];
  }
  return 0;
}

INT32 Cps1rPrepare()
{
  INT32 x; INT32 nAlign, nLWTiles;
  INT32 nTileYPos; INT32 nTileXPos;
  INT32 sy; INT32 iy; INT32 nYSize=16;
  INT32 nXSize=16;
  INT32 nPos;
  INT32 nPosX, nPosY;

  sy=16-(nCpsrScrY&15); iy=(nCpsrScrY>>4)+1;
  nCpstY=sy+(16<<4);

  nPosY=nCpstY; nPosX=nCpstX;
  nTileYPos=nPosY-16; nTileXPos=nPosX;

  // Find out how many rows and tiles are visible
  nLWTiles=400>>4; nAlign=400;

  nFirstY=iy; nLastY=iy+(224-1+16+16)/nYSize;

  for(y=iy-1; y<nLastY; y++)
  {
    INT32 nClipY=((y<iy)||(y>=nLastY)) ? 1 : 0;
    for(x=0, nPos=(y<<6)&0xfff; x<nLWTiles; x++, nPos+=2)
    {
      UINT32 b; UINT16 *ps;
      UINT32 t; UINT16 a;
      UINT32 bVCare = 0;
      INT32 p; INT32 nScrX; INT32 nScrY;
      INT32 nPix;
      UINT8 *pRow;

      if (nClipY)
        continue;

      nTileXPos=nPosX;
      if (nTileXPos>=(400-16)) 
        continue;

      // Get tile address
      p=*((UINT16 *)(CpsrBase + nPos));
      a=(UINT16)p;
      t=p>>16;

      if (Scroll2TileMask) t &= Scroll2TileMask;

      // Get tile data
      p=*((UINT16 *)(CpsrBase + nPos + 0x1000));
      nScrX=(p>>0) & 0x0f; nScrY=(p>>4) & 0x0f;
      t+=nCpsGfxScroll[2]; // add on offset to scroll tile

      ps=CpstPal;
      b=*((UINT16 *)(CpsrBase + nPos + 0x2000));
      a=(UINT16)a;

      nCpstX=nTileXPos; nCpstTile=t; nCpstFlip=(a>>5)&3;

      if (bVCare || x<0 || x>=nLWTiles-1) nCpstType=CTT_16X16 | CTT_CARE;
      else nCpstType=CTT_16X16;

      // Draw the tile
      CpstSetPal(a&0x1f);
      nPix=RenderTile(pRow, nCpstX, nTileYPos);
    }
  }

  return 0;
}

// Row scroll function
static INT32 nRowScroll[RENDER_ROW_ROW] = {0};

// For row scroll effects, fill up nRowScroll[] array up to RENDER_ROW_ROWMAX-1
static void GetRowScrollInfo(INT32 y)
{
  struct CpsrLineInfo *pli = &CpsrLineInfo[y >> 4];
  INT32 nOffs = nRowScroll[0];
  INT32 nWidth = pli->nWidth;
  INT32 i, j;

  // No row scroll for this line
  if (nWidth <= 0) return;

  // Get each scroll value (scaled to 256 pixels wide)
  for (i = 0, j = 0; i < 16; i++, j += nWidth)
  {
    nRowScroll[i + 1] = (pli->Rows[i] + nOffs) % 0x400;
  }
}

// CPS Row scroll and line effects
INT32 CpsrRowScroll(void)
{
  INT32 y;
  UINT16 *pDest = NULL;

  if (CpsrBase == NULL) {
    return 1;
  }

  // Offset to link to tilemap
  INT32 nLwTo = 0;
  INT32 nLwFrom = 0;
  
  // Add global scroll (for 1st tilemap)
  INT32 nScrX = nCpsrScrX - 0x40;
  INT32 nScrY = nCpsrScrY;
  
  for (y = 0; y < 32; y++)
  {
    struct CpsrLineInfo *pli = &CpsrLineInfo[y];
    
    // Find if there are any lines to draw
    if (pli->nWidth <= 0) continue;
    
    // Find out where in the destination the line would be drawn
    INT32 nY = (y << 4) - nScrY;
    
    // Draw it (if it's visible)
    if (nY < 0 || nY >= 224) continue;
    
    // Find where in the source to put it
    INT32 a = (nLwTo + nY) << 10;
    
    // Destination
    pDest = (UINT16*)(CpsrBase + a);
    
    // First fill in the line scroll info
    nRowScroll[0] = nScrX & 0x03FF;
    
    // Get line scroll info
    GetRowScrollInfo(y);
    
    // Savegame compatibility
    UINT16 MaskVal = 0;
    
    if(CpsSaveReg[0]) {
        // Cast pointers to uint8_t* first, then add, and cast result to UINT16* before dereferencing
        uint8_t* basePtr = (uint8_t*)CpsSaveReg[0];
        uint8_t* maskPtr = (uint8_t*)MaskAddr[(a & 0x180) >> 7];
        UINT16* finalPtr = (UINT16*)(basePtr + maskPtr);
        MaskVal = BURN_ENDIAN_SWAP_INT16(*finalPtr);
    }
    
    CpstPmsk = MaskVal;
    
    // Source to copy from is the same line of the 1st tilemap
    INT32 b = nLwFrom + nY;
    
    if (b < 0 || b >= RENDER_ROW_ROWMAX) continue;
    
    // Use nCpstType==1 for no flipping and no priorities
    INT32 nRealType = nCpstType;
    nCpstType = 1;
    
    nCpstX = -nRowScroll[1] + 64;
    nCpstY = nY;
    nCpstTile = b << 10;
    
    // Assign one value from pli->Rows (just the first one) to avoid type mismatch
    CpstRowShift = pli->Rows[0];
    
    if (CpstOneDoX[1])
        CpstOneDoX[1]();
    
    nCpstType = nRealType;
  }

  return 0;
}

// Render line scroll by filling solid with CPS1 rendering
static INT32 CpsrPmsk = 0;

INT32 CpsRowShift(void)
{
  if (CpsrBase == NULL) {
    return 1;
  }

  // Offset to link to tilemap
  INT32 nLwTo = 0;
  INT32 nScrX = nCpsrScrX;
  INT32 nScrY = nCpsrScrY;
  INT32 nWidth = RENDER_ROW_ROW;
  INT32 a, b, y;
  UINT16 *pDest = NULL;

  for (y = 0; y < 32; y++) 
  {
    // Find line info
    struct CpsrLineInfo *pli = &CpsrLineInfo[y];
    
    // Find if there are any lines to draw
    if (pli->nWidth <= 0) continue;
    
    // Find out where in the destination the line would be drawn
    INT32 nY = (y << 4) - nScrY;
    
    // Draw it (if it's visible)
    if (nY < 0 || nY >= 224) continue;

    // Find where in the source to put it
    a = (nLwTo + nY) << 10;
    
    // Destination
    pDest = (UINT16*)(CpsrBase + a);
    
    // Savegame compatibility
    if(CpsSaveReg[0]) {
        // Cast pointers to uint8_t* first, then add, and cast result to UINT16* before dereferencing
        uint8_t* basePtr = (uint8_t*)CpsSaveReg[0];
        uint8_t* maskPtr = (uint8_t*)MaskAddr[(a & 0x180) >> 7];
        UINT16* finalPtr = (UINT16*)(basePtr + maskPtr);
        CpstPmsk = BURN_ENDIAN_SWAP_INT16(*finalPtr);
    }
    
    // Source to copy from is the same line of the 1st tilemap
    b = nLwTo + nY;
    
    if (b < 0 || b >= RENDER_ROW_ROWMAX) continue;
    
    // Use nCpstType==1 for no flipping and no priorities
    INT32 nRealType = nCpstType;
    nCpstType = 1;
    
    nCpstX = nScrX;
    nCpstY = nY;
    nCpstTile = b;
    
    // Assign one value from pli->Rows (just the first one) to avoid type mismatch
    CpstRowShift = pli->Rows[0];
    
    if (CpstOneDoX[1])
        CpstOneDoX[1]();
    
    nCpstType = nRealType;
  }

  return 0;
}

// Scroll1 (total rows)
INT32 CpsrRender()
{
  INT32 nBlockWidth = 8;
  INT32 nScrWidth = 1024;
  INT32 nStartline = 0; // Define the nStartline variable that was missing
  INT32 nCpstAddY = 0;
  if (nScrWidth == 1536) nBlockWidth = 12;

  if (CpsrBase == NULL) {
    return 1;
  }

  // Offset to line scroll info
  INT32 nLink;
  // Number of 8pixel bands we need to draw from each tilemap to make a line
  INT32 nBlockTotal;
  // Pointer to tile data
  INT32 ps=0;
  
  UINT16 *pst= NULL;

  // (nRowMod): Amount to add to get to next row
  INT32 nRowMod = 0;

  // Get the start address of row data
  nLink = 0x400000;

  nCpstX = nCpsrScrX;
  nCpstY = nCpsrScrY;
  nCpstY += nCpstAddY;

  // Find which offset the draw function will start at
  nFirstY = (nStartline + (nCpsrScrY & 15)) >> 4;

  // Find out how many rows we have to draw
  nLastY = (nEndline + 16) >> 4;
  if (nFirstY < 0)
    nFirstY = 0;

  // Limit to row buffer size
  if (nLastY > RENDER_ROW_ROW)
    nLastY = RENDER_ROW_ROW;

  // Get number of blocks to copy from each row to make a whole line
  nBlockTotal = nScrWidth / nBlockWidth;

  // Work out the first block
  nFirstX = nCpsrScrX / nBlockWidth;
  // Work out how many blocks we need to draw
  nLastX = (nCpsrScrX + 384 + nBlockWidth - 1) / nBlockWidth;

  if (nFirstX < 0)
    nFirstX = 0;
  if (nLastX >= nBlockTotal)
    nLastX = nBlockTotal - 1;
  if (nLastX < nFirstX)
    return 0;

  for (INT32 iy = nFirstY; iy < nLastY; iy++)
  {
    INT32 nAddY;
    iy &= 31;
    nAddY = iy;
    nAddY <<= 5;

    for (INT32 ix = nFirstX; ix <= nLastX; ix++)
    {
      INT32 nAddX;
      UINT32 nTileAttr = 0;
      ix &= 127;
      nAddX = ix;

      ps = nLink + ((nAddY + nAddX) << 1);
      
      // Make a pointer to the source data for this tile
      pst = (UINT16 *)((uint8_t*)CpsrBase + ps);

      nTileAttr = BURN_ENDIAN_SWAP_INT16(pst[0]);

      nCpstTile = nTileAttr & 0x1FFF;
      if (nCpstTile >= 0x0080) nCpstTile += 0x2000;
      nCpstTile <<= 7;

      nCpstFlip = (nTileAttr >> 15) | ((nTileAttr >> 13) & 2);

      INT32 nPal = (nTileAttr >> 9) & 0x0f;
      nPal <<= 4;
      CpstSetPal(nPal);

      INT32 nClip = 0;

      INT32 nX = 64 + nBlockWidth * ix - (nCpsrScrX & 7);
      INT32 nY = iy << 4;
      nY -= nCpsrScrY & 15;

      INT32 bVCare = 0;
      if (iy <= 2 || iy >= (RENDER_ROW_ROW - 3))
      {
          if (iy == 0)
              nY -= 16;
          else if (iy == (RENDER_ROW_ROW - 1))
              nY += 16;
              
          bVCare = ((y << 4) < nStartline) | (((y << 4) + 16) >= nEndline);
      }

      if ((nX < -nBlockWidth) || (nX >= 384) || (nY < -16) || (nY >= 224 + 16))
        nClip = 1;

      if (((nX >= 384 - nBlockWidth) || (nX < 0)) || bVCare)
        nClip |= 2;

      nCpstX = nX;
      nCpstY = nY;
      nCpstType = nClip;

      if (CpstOneDoX[0])
          CpstOneDoX[0]();
    }
  }

  return 0;
}

// Make a scroll1 raster effect
// Create a sine table in nRowScroll
INT32 CpsrScratch(void)
{
  // Define the y variable explicitly here
  INT32 y;
  INT32 ix = nFirstX;
  INT32 iy = nFirstY;
  INT32 nLastY;

  iy = 0;
  nLastY = 60;

  // Find out which rows to draw
  if (iy < 0) iy = 0;
  if (nLastY > 32) nLastY = 32;
  if (iy >= nLastY) return 0;

  nCpstType = 1;
  
  for(y=iy-1; y<nLastY; y++)
  {
    INT32 nY = y << 4;
    INT32 nClipY=((y<iy)||(y>=nLastY)) ? 1 : 0;
    for(INT32 x=0, nPos=(y<<6)&0xfff; x<nLWTiles; x++, nPos+=2)
    {
      INT32 nX = (x << 5) + nRowScroll[y & 15];
      if ((nX < -31) || (nX >= 384)) continue;
      if (nX < 0 || nX > 351) nCpstType |= 2;
      else nCpstType &= ~2;

      nCpstX = nX;
      nCpstY = nY;
      nCpstTile = nPos;
      nCpstFlip = 0;
      CpstSetPal(0);

      if (CpstOneDoX[0])
          CpstOneDoX[0]();
    }
  }

  return 0;
}
