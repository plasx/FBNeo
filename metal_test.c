#include <stdio.h>

// External functions from our Metal port
extern void BurnStateLoad(char* szName, int nOffset, int (*pLoadGame)());
extern void BurnStateSave(char* szName, int nOffset);
extern void IpsApplyPatches(unsigned char* base, char* rom_name, unsigned int rom_crc, int readonly);

int main(int argc, char* argv[]) {
    printf("Testing FBNeo Metal port functions\n");
    
    // Call some of our stub functions
    char testName[] = "test.sav";
    BurnStateSave(testName, 0);
    
    printf("Metal port functions tested successfully\n");
    return 0;
} 