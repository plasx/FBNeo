#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rom_loading_debug.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <romfile.zip>\n", argv[0]);
        return 1;
    }
    
    ROMLoader_InitDebugLog();
    ROMLoader_SetDebugLevel(LOG_VERBOSE);
    ROMLoader_DebugLog(LOG_INFO, "Analyzing ROM file: %s", argv[1]);
    
    ROMLoader_LogROMInfo(argv[1]);
    
    return 0;
}
