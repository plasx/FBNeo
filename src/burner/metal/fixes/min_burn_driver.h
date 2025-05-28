// Minimal BurnDriver struct and definitions for burn.cpp
#ifndef MIN_BURN_DRIVER_H
#define MIN_BURN_DRIVER_H

// Forward declare or define any types needed
typedef char TCHAR;
typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int UINT32;
typedef int INT32;

// Simplified BurnDriver struct with just the fields needed for compilation
struct BurnDriver {
    const TCHAR* szShortName;
    const TCHAR* szParent;
    const TCHAR* szBoardROM;
    const TCHAR* szAllRomsAllSoftwareRegionAllDisks;
    const TCHAR* szDate;
    const TCHAR* szFullNameA;
    const TCHAR* szGlueTitle;
    const TCHAR* szCommentA;
    const TCHAR* szManufacturerA;
    const TCHAR* szSystemA;
    const TCHAR* szFullNameW;
    const TCHAR* szCommentW;
    const TCHAR* szManufacturerW;
    const TCHAR* szSystemW;
    INT32 nGenre;
    INT32 nFamily;
    INT32 nFlags;
    INT32 nMaxPlayers;
    INT32 nWidth;
    INT32 nHeight;
    INT32 nXAspect;
    INT32 nYAspect;
    INT32 nScrnFlags;
    // Function pointers - defined as just void*
    void* pDriverCallback;
    void* pGetZipName;
    void* pGetRomInfo;
    void* pGetRomName;
    void* pGetSampleInfo;
    void* pGetSampleName;
    void* pGetInputInfo;
    void* pGetDIPInfo;
    void* pInit;
    void* pExit;
    void* pFrame;
    void* pDraw;
    void* pScan;
    void* pSetColorTable;
};

// Declare pDriver extern
extern struct BurnDriver* pDriver[];

#endif // MIN_BURN_DRIVER_H
