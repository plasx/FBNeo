#ifndef cps_h
#define cps_h

#include "burnint.h"
#include "m68000_intf.h"
#include "z80_intf.h"
#include "msm6295.h"
#include "timer.h"
#include "samples.h"
#include "burn/devices/eeprom.h"

// Include CPS stub header for Metal implementation
#ifdef CPS_STUB_H
#include "cps_stub.h"
#endif

// Include Metal fixes for Metal builds
#ifdef USE_METAL_FIXES
#include "metal_fixes.h"
#endif

#ifndef CPS2_PRG_68K
#define CPS2_PRG_68K  0x01
#endif
#ifndef CPS2_GFX
#define CPS2_GFX      0x02
#endif
#ifndef CPS2_PRG_Z80
#define CPS2_PRG_Z80  0x03
#endif
#ifndef CPS2_QSND
#define CPS2_QSND     0x04
#endif
#ifndef CPS2_ENCRYPTION_KEY
#define CPS2_ENCRYPTION_KEY 0x05
#endif
#ifndef CPS2_PRG_68K_SIMM
#define CPS2_PRG_68K_SIMM 0x06
#endif
#ifndef CPS2_PRG_68K_XOR_TABLE
#define CPS2_PRG_68K_XOR_TABLE 0x07
#endif
#ifndef CPS2_GFX_SIMM
#define CPS2_GFX_SIMM 0x08
#endif
#ifndef CPS2_GFX_SPLIT4
#define CPS2_GFX_SPLIT4 0x09
#endif
#ifndef CPS2_GFX_SPLIT8
#define CPS2_GFX_SPLIT8 0x0A
#endif
#ifndef CPS2_GFX_19XXJ
#define CPS2_GFX_19XXJ CPS2_GFX
#endif
#ifndef CPS2_QSND_SIMM
#define CPS2_QSND_SIMM 0x0B
#endif
#ifndef CPS2_QSND_SIMM_BYTESWAP
#define CPS2_QSND_SIMM_BYTESWAP 0x0C
#endif

// global variables used by CPS drivers
extern INT32 Cps;								// 1 = CPS1 hardware, 2 = CPS2 hardware
extern INT32 Cps1Qs;							// 1 = CPS1 with QSound
extern INT32 Cps1Run;							// 1 = CPS1 Classic, 2 = CPS1 with QSound

// To be defined by the CPS driver
extern UINT8 *CpsGfx; extern UINT32 nCpsGfxLen; // All the graphics
extern UINT8 *CpsRom; extern UINT32 nCpsRomLen; // Program Rom (as in rom)
extern UINT8 *CpsCode; extern UINT32 nCpsCodeLen; // Mapped Program Rom
extern UINT8 *CpsZRom; extern UINT32 nCpsZRomLen; // Z80 Roms
extern UINT8 *CpsQSam; extern UINT32 nCpsQSamLen; // QSound Sample Roms
extern UINT8 *CpsAd; extern UINT32 nCpsAdLen;     // ADPCM data
extern UINT8 *CpsStar; extern UINT32 nCpsStarLen; // Star layer graphics
extern UINT8 *CpsText; extern UINT32 nCpsTextLen; // Text layer graphics
extern UINT8 *CpsKey; // Key data for CPS2 decryption
extern INT32 CpsGfxScroll1; extern INT32 CpsGfxScroll2; extern INT32 CpsGfxScroll3; extern INT32 CpsGfxObject; extern INT32 CpsGfxStars;
extern UINT32 CpsGfxScroll1Mask; extern UINT32 CpsGfxScroll2Mask; extern UINT32 CpsGfxScroll3Mask; extern UINT32 CpsGfxObjectMask; extern UINT32 CpsGfxStarsMask;
extern INT32 CpsGfxScroll1Shift; extern INT32 CpsGfxScroll2Shift; extern INT32 CpsGfxScroll3Shift; extern INT32 CpsGfxObjectShift; extern INT32 CpsGfxStarsShift;
extern UINT32 nCpsGfxMask;

// Maximum number of beam-synchronized interrupts to check
#define MAX_RASTER 16

#ifdef __cplusplus
extern "C" {
#endif

// Function Prototypes

// cps.cpp
INT32 CpsInit();
INT32 Cps2Init();
INT32 CpsExit();

INT32 CpsLoadTiles(UINT8 *Tile,INT32 nStart);
INT32 CpsLoadTilesByte(UINT8 *Tile,INT32 nStart);
INT32 CpsLoadTilesForgottn(INT32 nStart);
INT32 CpsLoadTilesForgottna(INT32 nStart);
INT32 CpsLoadTilesForgottnu(INT32 nStart);
INT32 CpsLoadTilesSf2ebbl(UINT8 *Tile, INT32 nStart);
INT32 CpsLoadTilesSf2b(UINT8 *Tile, INT32 nStart);
INT32 CpsLoadTilesSf2koryuExtra(UINT8 *Tile, INT32 nStart);
INT32 CpsLoadTilesSf2mkotExtra(UINT8 *Tile, INT32 nStart);
INT32 CpsLoadTilesHack160(INT32 nStart);
INT32 CpsLoadTilesHack160Alt(INT32 nStart);
INT32 CpsLoadTilesSf2koryu(INT32 nStart);
INT32 CpsLoadTilesSf2stt(INT32 nStart);
INT32 CpsLoadTilesSf2mdt(INT32 nStart);
INT32 CpsLoadTilesSf2mdta(INT32 nStart);
INT32 CpsLoadTilesSf2m8(INT32 nStart);
INT32 CpsLoadTilesSf2ceeabl(INT32 nStart);
INT32 CpsLoadTilesSf2ceblp(INT32 nStart);
INT32 CpsLoadTilesSf2ebbl3(INT32 nStart);
INT32 CpsLoadTilesSf2amf10(INT32 nStart);
INT32 CpsLoadTilesFcrash(INT32 nStart);
INT32 CpsLoadTilesCawingbl(INT32 nStart);
INT32 CpsLoadTilesCaptcommb(INT32 nStart);
INT32 CpsLoadTilesDinopic(INT32 nStart);
INT32 CpsLoadTilesDinopic5(INT32 nStart);
INT32 CpsLoadTilesSlampic(INT32 nStart);
INT32 CpsLoadTilesKodb(INT32 nStart);
INT32 CpsLoadTilesWonder3b(INT32 nStart);
INT32 CpsLoadTilesPang3(INT32 nStart);
INT32 CpsLoadTilesPang3r1a(INT32 nStart);
INT32 CpsLoadTilesPang3b2(INT32 nStart);
INT32 CpsLoadTilesPang3b4(INT32 nStart);
INT32 CpsLoadTilesPang3b5(INT32 nStart);
INT32 CpsLoadTilesGulunpa(INT32 nStart);
INT32 CpsLoadTilesPunisherb(INT32 nStart);
INT32 CpsLoadTilesKnightsb2(INT32 nStart);
INT32 CpsLoadTilesMtwinsb(INT32 nStart);
INT32 CpsLoadTilesWofabl(INT32 nStart);
INT32 CpsLoadStars(UINT8 *pStar, INT32 nStart);
INT32 CpsLoadStarsByte(UINT8 *pStar, INT32 nStart);
INT32 CpsLoadStarsForgottnAlt(UINT8 *pStar, INT32 nStart);
INT32 Cps2LoadTiles(UINT8 *Tile,INT32 nStart);
INT32 Cps2LoadTilesSIM(UINT8 *Tile,INT32 nStart);
INT32 Cps2LoadTilesGigaman2(UINT8 *Tile, UINT8 *pSrc);

#ifdef __cplusplus
}
#endif

// cps_config.h
#define CPS_B_01		0
#define CPS_B_02		1
#define CPS_B_03		2
#define CPS_B_04		3
#define CPS_B_05		4
#define CPS_B_11		5
#define CPS_B_12		6
#define CPS_B_13		7
#define CPS_B_14		8
#define CPS_B_15		9
#define CPS_B_16		10
#define CPS_B_17		11
#define CPS_B_18		12
#define CPS_B_21_DEF		13
#define CPS_B_21_BT1		14
#define CPS_B_21_BT2		15
#define CPS_B_21_BT3		16
#define CPS_B_21_BT4		17
#define CPS_B_21_BT5		18
#define CPS_B_21_BT6		19
#define CPS_B_21_BT7		20
#define CPS_B_21_QS1		21
#define CPS_B_21_QS2		22
#define CPS_B_21_QS3		23
#define CPS_B_21_QS4		24
#define CPS_B_21_QS5		25
#define HACK_B_1		26
#define HACK_B_2		27
#define HACK_B_3		28
#define HACK_B_4		29
#define HACK_B_5		30
#define HACK_B_6		31

#define GFXTYPE_SPRITES		(1<<0)
#define GFXTYPE_SCROLL1		(1<<1)
#define GFXTYPE_SCROLL2		(1<<2)
#define GFXTYPE_SCROLL3		(1<<3)
#define GFXTYPE_STARS		(1<<4)

#define mapper_LWCHR		0
#define mapper_LW621		1
#define mapper_DM620		2
#define mapper_ST24M1		3
#define mapper_DM22A		4
#define mapper_DAM63B		5
#define mapper_ST22B		6
#define mapper_TK22B		7
#define mapper_WL24B		8
#define mapper_S224B		9
#define mapper_YI24B		10
#define mapper_AR24B		11
#define mapper_AR22B		12
#define mapper_O224B		13
#define mapper_MS24B		14
#define mapper_CK24B		15
#define mapper_NM24B		16
#define mapper_CA24B		17
#define mapper_CA22B		18
#define mapper_STF29		19
#define mapper_RT24B		20
#define mapper_RT22B		21
#define mapper_KD29B		22
#define mapper_CC63B		23
#define mapper_KR63B		24
#define mapper_S9263B		25
#define mapper_VA63B		26
#define mapper_VA22B		27
#define mapper_Q522B		28
#define mapper_TK263B		29
#define mapper_CD63B		30
#define mapper_PS63B		31
#define mapper_MB63B		32
#define mapper_QD22B		33

#ifdef __cplusplus
extern "C" {
#endif

void SetGfxMapper(INT32 MapperId);
INT32 GfxRomBankMapper(INT32 Type, INT32 Code);
void SetCpsBId(INT32 CpsBId, INT32 bStars);

INT32 CpsPalInit();
INT32 CpsPalExit();
INT32 CpsPalUpdate(UINT8 *pNewPal);

void CpsMapObjectBanks(INT32 nBank);
INT32 CpsMemInit();
INT32 CpsMemExit();
INT32 CpsAreaScan(INT32 nAction,INT32 *pnMin);

INT32 CpsRunInit();
INT32 CpsRunExit();
INT32 Cps1Frame();
INT32 Cps2Frame();

INT32 CpsRwInit();
INT32 CpsRwExit();
void CpsRwScan();
INT32 CpsRwGetInp();
void CpsWritePort(const UINT32 ia, UINT8 d);
UINT8 __fastcall CpsReadByte(UINT32 a);
void __fastcall CpsWriteByte(UINT32 a, UINT8 d);
UINT16 __fastcall CpsReadWord(UINT32 a);
void __fastcall CpsWriteWord(UINT32 a, UINT16 d);

void DrawFnInit();
INT32  CpsDraw();
INT32  CpsRedraw();

INT32 QsndInit();
void QsndSetRoute(INT32 nIndex, double nVolume, INT32 nRouteDir);
void QsndNewFrame();
void QsndEndFrame();
#if defined USE_SPEEDHACKS
void QsndSyncZ80();
#else
INT32 QsndSyncZ80();
#endif
INT32 QsndScan(INT32 nAction);

INT32 QsndZInit();
INT32 QsndZExit();
INT32 QsndZScan(INT32 nAction);

INT32 QscInit(INT32 nRate);
void QscSetRoute(INT32 nIndex, double nVolume, INT32 nRouteDir);
void QscReset();
void QscExit();
INT32 QscScan(INT32 nAction);
void QscNewFrame();
void QscWrite(INT32 a, INT32 d);
UINT8 QscRead();
INT32 QscUpdate(INT32 nEnd);

#ifdef __cplusplus
}
#endif

// CpsOneDoX functions
INT32 CpsOneDoX();
INT32 CpsBgOneDoX();
INT32 CpsRowOneDoX();
INT32 Cps2OneDoX();

// Cpsr functions/variables that whole driver needs
extern UINT8 *CpsrBase;			// Tile data base
extern INT32 nCpsrScrX, nCpsrScrY;	// Basic scroll info
extern UINT16 *CpsrRows;		// Row scroll table, 0x400 words long
extern INT32 nCpsrRowStart;		// Start of row scroll (can be negative)

extern INT32 CpsrBgHack;

extern INT32 nEndline;

// Prototype for Cpst function
INT32 Cps1Tile16(void);
INT32 Cps1Tile16_Metal(void);
INT32 Cps2Tile16(void);
INT32 Cps2Tile32(void);
INT32 Cps2FastTile32(void);

// CpsrLineInfo structure definition for row scroll
struct CpsrLineInfo {
    INT32 nStart;       // Start position
    INT32 nWidth;       // Width
    INT32 nTileStart;   // Tile Start position
    INT32 nTileEnd;     // Tile End position
    INT16 Rows[16];     // Row shift data
    INT32 nMaxLeft;     // Maximum row scroll left
    INT32 nMaxRight;    // Maximum row scroll right
};

extern struct CpsrLineInfo CpsrLineInfo[32];

// Palette stuff
extern INT32 nCpsPalCtrlReg;
extern INT32 bCpsUpdatePalEveryFrame;

// Screen Control
#define CPSSCR_DARK(x)		((x >> 6) & 3)

// Handle CPS_STUB_H implementation differently
#ifndef CPS_STUB_H
extern UINT16 nCpstPal;
inline static void CpstSetPal(INT32 nPal)
{
    nCpstPal = nPal;
}
#else
// CpstSetPal is defined in cps_stub.h
#endif

void CpsFastVidCheck();
void CpsFastVidDefault();

#endif // cps_h
