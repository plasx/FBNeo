//This file is no longer needed since proper YM2151 implementations
//are now provided in the main burn_ym2151.cpp file.
//
//The zero-argument versions of functions have been added directly to burn_ym2151.cpp/h.
//
//This file is kept as a placeholder in case additional wrappers are needed in the future. 

#include "burnint.h"
#include "burn_ym2151.h"
#include "ym2151.h"
#include <string.h>
#include <stdlib.h>

typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int UINT32;
typedef int INT32;

// External access to YM2151 internal state
extern struct YM2151Chip {
    INT32 nRegister;
    INT32 nStatus;
} YM2151Chip[2];

extern INT32 nYM2151Volume;
extern INT32 nNumChips;
extern UINT8 DebugSnd_YM2151Initted;

// Proper implementation for BurnYM2151Exit
extern "C" {
    void BurnYM2151Exit() {
        if (!DebugSnd_YM2151Initted) return;
        
        // Call YM2151Shutdown with no arguments per current header
        YM2151Shutdown();
        
        // Reset state
        nYM2151Volume = 0;
        nNumChips = 0;
        DebugSnd_YM2151Initted = 0;
    }
    
    // Implementation of BurnYM2151Reset function
    void BurnYM2151Reset() {
        if (!DebugSnd_YM2151Initted) return;
        
        // Reset each YM2151 chip - use proper argument per header
        for (INT32 i = 0; i < nNumChips; i++) {
            YM2151ResetChip(i);
        }
    }
    
    // Implementation of BurnYM2151Scan function with correct signature
    void BurnYM2151Scan(INT32 nAction, INT32 *pnMin) {
        if (!DebugSnd_YM2151Initted) return;
        
        if ((nAction & ACB_DRIVER_DATA) == 0)
            return;
            
        // Scan variable information for each chip
        SCAN_VAR(YM2151Chip[0]);
        if (nNumChips > 1) {
            SCAN_VAR(YM2151Chip[1]);
        }
    }
    
    // Implementation of BurnYM2151Write function
    void BurnYM2151Write(INT32 nChip, INT32 nAddress, UINT8 nData) {
        if (!DebugSnd_YM2151Initted) return;
        
        if (nChip < nNumChips) {
            if (nAddress & 1) {
                // Pass chip number as first argument
                YM2151WriteReg(nChip, YM2151Chip[nChip].nRegister, nData);
            } else {
                YM2151Chip[nChip].nRegister = nData;
            }
        }
    }
    
    // Implementation of BurnYM2151Read function with the proper signature
    UINT8 BurnYM2151Read(INT32 nChip) {
        if (!DebugSnd_YM2151Initted) return 0;
        
        if (nChip < nNumChips) {
            // Pass chip number as argument
            return YM2151ReadStatus(nChip);
        }
        
        return 0;
    }
} 