#include "burnint.h"

// CPS functions
int CpsInit() { return 0; }
int CpsExit() { return 0; }
int Cps1Scan(int nAction, int* pnMin) { return 0; }
int Cps2Scan(int nAction, int* pnMin) { return 0; }
int Cps3Scan(int nAction, int* pnMin) { return 0; }
int CpsAreaScan(int nAction, int* pnMin) { return 0; }
int CpsRunInit() { return 0; }
int CpsRunExit() { return 0; }
int CpsRunFrame() { return 0; }
int Cps1Frame() { return 0; }
int Cps2Frame() { return 0; }
int CpsDrawScanline(int nScanline) { return 0; }
void CpsPalUpdate(unsigned char* pNewPal) { }
void Cps1Layers() { }
void Cps2Layers() { }
void CpsObjInit() { }
void CpsObjDrawInit() { }
int CpsMemInit() { return 0; }
int CpsMemExit() { return 0; }
int CpsLoadOne(unsigned char* Tile, int nNum, int nWord, int nShift) { return 0; }
int CpsLoadOneHack160(unsigned char* Tile, int nNum, int nWord, int nShift) { return 0; }
int CpsLoadOneSf2koryu(unsigned char* Tile, int nNum, int nWord, int nShift) { return 0; }
int CpsLoadOneSf2ebbl(unsigned char* Tile, int nNum, int nWord, int nShift) { return 0; }
int CpsLoadOneSf2b(unsigned char* Tile, int nNum, int nWord, int nShift) { return 0; }
int CpsLoadOneSf2stt(unsigned char* Tile, int nNum, int nWord, int nShift) { return 0; }
unsigned char CpsReadPort(unsigned int a) { return 0; }
void CpsWritePort(unsigned int a, unsigned char d) { }
void CpsSoundCmd(unsigned short a) { }
void DrawFnInit() { }
void DrawScroll2Init(int a) { }
int CpsObjGet() { return 0; }
void Cps1ObjDraw(int nLevelFrom, int nLevelTo) { }
void Cps2ObjDraw(int nLevelFrom, int nLevelTo) { }
void FcrashObjDraw(int nLevelFrom, int nLevelTo) { }
void Cps1Scr1Draw(unsigned char* pSrc, int nLevelFrom, int nLevelTo) { }
void Cps1Scr3Draw(unsigned char* pSrc, int nLevelFrom, int nLevelTo) { }
void Cps2Scr1Draw(unsigned char* pSrc, int nLevelFrom, int nLevelTo) { }
void Cps2Scr3Draw(unsigned char* pSrc, int nLevelFrom, int nLevelTo) { }
INT32 CpsMemIndex() { return 0; }
void CpsPalInit() { }
void EcofghtExit() { }
void DrvReset() { }

// CPS/CPS2 crypto helper functions
void cps2_decrypt_game_data() { }
int CpsQSoundCheatSearchCallback() { return 0; }

// Aspect ratio function
int check_aspect() { return 0; }
