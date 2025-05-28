#include "burnint.h"
#include "m68000_intf.h"

// These are the missing symbols from the M68K interface
struct SekExt *SekExt[SEK_MAX] = { NULL, }, *pSekExt = NULL;

// Additional minimal M68K function implementations
unsigned short CpsReadWord(unsigned int a) { return 0; }
void CpsWriteWord(unsigned int a, unsigned short d) { }
void PhoenixOutputWriteWord(unsigned int a, unsigned short d) { }
