#include "burnint.h"

// ROM info functions for Ninexx
struct BurnRomInfo* NinexxRomInfo(struct BurnRomInfo* pri, unsigned int i) { return pri; }
char** NinexxRomName(char** pszName, unsigned int i, int nAka) { return pszName; }
struct BurnRomInfo* NinexxuRomInfo(struct BurnRomInfo* pri, unsigned int i) { return pri; }
char** NinexxuRomName(char** pszName, unsigned int i, int nAka) { return pszName; }
struct BurnRomInfo* NinexxaRomInfo(struct BurnRomInfo* pri, unsigned int i) { return pri; }
char** NinexxaRomName(char** pszName, unsigned int i, int nAka) { return pszName; }
struct BurnRomInfo* Ninexxar1RomInfo(struct BurnRomInfo* pri, unsigned int i) { return pri; }
char** Ninexxar1RomName(char** pszName, unsigned int i, int nAka) { return pszName; }
struct BurnRomInfo* NinexxbRomInfo(struct BurnRomInfo* pri, unsigned int i) { return pri; }
char** NinexxbRomName(char** pszName, unsigned int i, int nAka) { return pszName; }
struct BurnRomInfo* NinexxhRomInfo(struct BurnRomInfo* pri, unsigned int i) { return pri; }
char** NinexxhRomName(char** pszName, unsigned int i, int nAka) { return pszName; }
struct BurnRomInfo* NinexxjRomInfo(struct BurnRomInfo* pri, unsigned int i) { return pri; }
char** NinexxjRomName(char** pszName, unsigned int i, int nAka) { return pszName; }
struct BurnRomInfo* Ninexxjr1RomInfo(struct BurnRomInfo* pri, unsigned int i) { return pri; }
char** Ninexxjr1RomName(char** pszName, unsigned int i, int nAka) { return pszName; }
struct BurnRomInfo* Ninexxjr2RomInfo(struct BurnRomInfo* pri, unsigned int i) { return pri; }
char** Ninexxjr2RomName(char** pszName, unsigned int i, int nAka) { return pszName; }
struct BurnRomInfo* NinexxdRomInfo(struct BurnRomInfo* pri, unsigned int i) { return pri; }
char** NinexxdRomName(char** pszName, unsigned int i, int nAka) { return pszName; }

// Other ROM functions that might be needed
struct BurnRomInfo* MvscRomInfo(struct BurnRomInfo* pri, unsigned int i) { return pri; }
char** MvscRomName(char** pszName, unsigned int i, int nAka) { return pszName; }

// QSound stubs
int QscRead() { return 0; }
int QscUpdate(int a) { return 0; }
int QsndEndFrame() { return 0; }
