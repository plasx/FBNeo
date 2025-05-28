#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include required headers
#include "fixes/c_cpp_compatibility.h"
#include "metal_declarations.h"

// External functions from our ROM verification modules
extern "C" {
    void ROMLoader_InitDebugLog();
    void ROMLoader_DebugLog(int level, const char* format, ...);
    void ROMLoader_LogROMInfo(const char* romPath);
    int Metal_VerifyGameROM(const char* gameName);
    int Metal_DumpZipContents(const char* zipPath, const char* gameName);
    int Metal_DiagnoseROMLoading(const char* romPath);
    
    // Stub implementations for required FBNeo functions
    int BurnDrvGetIndexByName(const char* szName) {
        // Basic implementation to allow diagnostics to run
        // For real verification, the FBNeo core is needed
        static const char* knownGames[] = {
            "mvsc", "sfa3", "sf2ce", "ssf2t", "dino", "ddtod", 
            "nwarr", "xmvsf", "msh", "mshvsf", NULL
        };
        
        if (!szName || !*szName) return -1;
        
        for (int i = 0; knownGames[i] != NULL; i++) {
            if (strcasecmp(szName, knownGames[i]) == 0) {
                return i;
            }
        }
        
        return -1;
    }
    
    int BurnDrvSelect(int nDriver) {
        return 0; // Always succeed for diagnostics
    }
    
    const char* BurnDrvGetTextA(int iIndex) {
        static char buffer[256];
        snprintf(buffer, sizeof(buffer), "Game Driver #%d", iIndex);
        return buffer;
    }
    
    int BurnDrvGetRomInfo(void* pri, int i) {
        return -1; // Not implemented in standalone diagnostic
    }
    
    int BurnDrvGetRomName(char* szName, int i, int j) {
        if (szName) *szName = 0;
        return -1; // Not implemented in standalone diagnostic
    }
    
    int BurnDrvGetZipName(char** pszName, int i) {
        static char buffer[256];
        if (i == 0) {
            strcpy(buffer, "mvsc");
            *pszName = buffer;
            return 0;
        }
        return -1; // No more ZIP names
    }
}

// Print usage information
void printUsage(const char* programName) {
    printf("Usage: %s [options] <rom_file.zip>\n", programName);
    printf("\n");
    printf("Options:\n");
    printf("  --verify     Verify ROM against FBNeo driver\n");
    printf("  --dump       Dump ZIP contents\n");
    printf("  --diagnose   Run full diagnostics\n");
    printf("  --help       Show this help message\n");
    printf("\n");
    printf("Example: %s --diagnose mvsc.zip\n", programName);
}

// Main program
int main(int argc, char** argv) {
    printf("FBNeo ROM Diagnostic Tool\n");
    printf("========================\n\n");
    
    // Check if we have enough arguments
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }
    
    // Parse command line
    const char* romPath = NULL;
    enum {
        ACTION_VERIFY,
        ACTION_DUMP,
        ACTION_DIAGNOSE
    } action = ACTION_VERIFY; // Default action
    
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            // Option
            if (strcmp(argv[i], "--verify") == 0) {
                action = ACTION_VERIFY;
            } else if (strcmp(argv[i], "--dump") == 0) {
                action = ACTION_DUMP;
            } else if (strcmp(argv[i], "--diagnose") == 0) {
                action = ACTION_DIAGNOSE;
            } else if (strcmp(argv[i], "--help") == 0) {
                printUsage(argv[0]);
                return 0;
            } else {
                printf("Unknown option: %s\n", argv[i]);
                printUsage(argv[0]);
                return 1;
            }
        } else {
            // ROM path
            romPath = argv[i];
        }
    }
    
    // Make sure we have a ROM path
    if (!romPath) {
        printf("Error: No ROM file specified\n");
        printUsage(argv[0]);
        return 1;
    }
    
    // Initialize logging
    ROMLoader_InitDebugLog();
    ROMLoader_DebugLog(0, "FBNeo ROM Diagnostic Tool started");
    ROMLoader_DebugLog(0, "ROM path: %s", romPath);
    
    // Get ROM filename without path
    const char* fileName = strrchr(romPath, '/');
    if (fileName) {
        fileName++; // Skip the slash
    } else {
        fileName = romPath;
    }
    
    // Create a copy of the filename for manipulation
    char baseFileName[512];
    strncpy(baseFileName, fileName, sizeof(baseFileName) - 1);
    baseFileName[sizeof(baseFileName) - 1] = '\0';
    
    // Remove the extension if present
    char* dot = strrchr(baseFileName, '.');
    if (dot) {
        *dot = '\0';
    }
    
    // Perform the requested action
    int result = 0;
    
    switch (action) {
        case ACTION_VERIFY:
            printf("Verifying ROM: %s\n", romPath);
            ROMLoader_DebugLog(0, "Verifying ROM: %s (base name: %s)", romPath, baseFileName);
            // Simple verification for diagnostic mode
            result = Metal_VerifyGameROM(baseFileName);
            printf("Verification result: %s\n", result == 0 ? "SUCCESS" : "FAILED");
            break;
            
        case ACTION_DUMP:
            printf("Dumping ROM contents: %s\n", romPath);
            ROMLoader_DebugLog(0, "Dumping ROM contents: %s", romPath);
            result = Metal_DumpZipContents(romPath, baseFileName);
            printf("Dump complete\n");
            break;
            
        case ACTION_DIAGNOSE:
            printf("Running diagnostics on ROM: %s\n", romPath);
            ROMLoader_DebugLog(0, "Running full diagnostics on ROM: %s", romPath);
            result = Metal_DiagnoseROMLoading(romPath);
            printf("Diagnostic complete, check log for details\n");
            break;
    }
    
    // Final result
    ROMLoader_DebugLog(0, "Diagnostic tool completed with result: %d", result);
    printf("See rom_loading_debug.log for detailed information\n");
    
    return result;
} 