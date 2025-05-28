// metal_linkage_bridge.cpp
// This file ensures all stub functions have proper linkage in the final executable

#include <string.h> // For strncpy
#include <stdbool.h> // For bool type

// Define the struct type needed for tms34010_generate_scanline
struct _tms34010_display_params { int dummy; };

// External variables
extern "C" {
    // EEPROM path global
    char szAppEEPROMPath[260] = {0}; // Must be defined, not just declared

    // TMS34010 functions
    int tms34010_generate_scanline(int line, int (*callback)(int, struct _tms34010_display_params*)) { 
        return 0; 
    }

    // BurnState functions
    int BurnStateLoad(char* szName, int nOffset, int (*pLoadGame)()) { 
        return 0; 
    }
    
    int BurnStateSave(char* szName, int nOffset) { 
        return 0; 
    }

    // QSound functions 
    int QsndScan(int nAction) { 
        return 0; 
    }
    
    void QsndSyncZ80() {
        // Empty implementation
    }

    // BurnSample functions
    void BurnSampleRender_INT(unsigned int nSegmentLength) {
        // Empty implementation
    }

    // String conversion functions
    void TCHARToANSI(const char* pszInString, char* pszOutString, int nOutSize) {
        if (pszOutString && nOutSize > 0) {
            strncpy(pszOutString, pszInString, nOutSize - 1);
            pszOutString[nOutSize - 1] = 0;
        }
    }

    // 68K callbacks
    void m68k_modify_timeslice(int value) {
        // Empty implementation
    }
    
    // IPS Patches
    void IpsApplyPatches(unsigned char* base, char* rom_name, unsigned int rom_crc, bool readonly) {
        // Empty implementation
    }
} 