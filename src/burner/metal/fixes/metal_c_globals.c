/* metal_c_globals.c - Global variables with proper C linkage for the Metal port */

/* Basic types */
typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int UINT32;
typedef int INT32;
typedef char TCHAR;

/* Function pointer type for CTV drawing functions */
typedef INT32 (*CtvDoFn)();

/* CPS globals */
UINT32 *CpstPal = 0;
UINT16 ZValue = 0;
UINT16 *ZBuf = 0;
UINT16 *pZVal = 0;
INT32 nBgHi = 0;

/* ROM and driver data */
void* pDataRomDesc = 0;
void* pRDI = 0;
INT32 nLanguage = 0;
UINT32 nTilesSize = 0;
UINT32 LwsLen = 0;
UINT32 nIpsDrvDefine = 0;

/* Sound */
UINT8 bPsndOkay = 0;
UINT8 QsndOkay = 0;
UINT8 AY8910Init = 0;

/* IPS patches */
UINT32 nIpsMemExpLen = 0;
UINT32 nExtraLen = 0;

/* Animation */
UINT32 nAnimCount = 0;
UINT32 nAnimFrames = 0;

/* QSound */
UINT16 QscCmd = 0;
UINT8 QsndZReset = 0;
UINT8 QsndZBank = 0;
UINT8 QsndZFlag = 0;
UINT8 QsndZMaster = 0;
UINT8 QscRomPresent = 0;
UINT32 nQscLen = 0;

/* Misc */
char _StringBuf[1024];
UINT8 *pSekExt = 0;

/* MIPS */
UINT32 *pMipsMemFetch = 0;
UINT32 *pMipsMemOpbase = 0;
UINT32 *pMipsmem = 0;
void *config = 0;

/* Neogeo */
UINT32 nNeoTextROMSize = 0;
UINT32 nNeoSpriteROMSize = 0;
UINT8 *Neo68KROMActive = 0;
UINT8 nWhichGame = 0;
UINT8 bNeoSpriteFrame = 0;

/* Utility */
UINT32 __dummy = 0;

/* MSM6295 */
UINT32 nMSM6295Intf = 0;
UINT32 nMSM6295Volume = 0;
UINT32 nMSM6295BankShift = 0;

/* CTV */
CtvDoFn CtvDoX[32] = {0};
CtvDoFn CtvDoXM[32] = {0};
CtvDoFn CtvDoXB[32] = {0};
UINT32 nCtvRollX = 0;
UINT32 nCtvRollY = 0;
UINT8 *pCtvTile = 0;
INT32 nCtvTileAdd = 0;
UINT8 *pCtvLine = 0;

/* Z80 specific globals */
unsigned char Z80Vector = 0;

// QSound globals
int QsndScan(int nAction, int* pnMin) {
    // Stub implementation
    return 0;
}

// CTV coordinate globals
int nCpstPosX = 0;
int nCpstPosY = 0;

// BurnState globals
int BurnStateLoad(char* szName, int bAll, int (*pLoadGame)()) {
    // Stub implementation
    return 0;
}

int BurnStateSave(char* szName, int bAll) {
    // Stub implementation
    return 0;
}

// Active driver index
int nBurnDrvActive = -1;
