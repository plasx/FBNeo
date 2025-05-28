#include "burnint.h"
#include <string.h>
#include <stdlib.h>

// EEPROM related variables
typedef struct {
    INT32 nDeviceType;
    INT32 nAddressBits;
    INT32 nDataBits;
    INT32 nSettings;
    INT32 nAddressMask;
    INT32 nDataMask;
    
    UINT32 nCurrentAddress;
    INT32 nCurrentAddressMask;
    UINT32 nReadAddress;
    
    INT32 nReadBit;
    INT32 nReadWriteBit;
    INT32 nClockBit;
    INT32 nEEPROMOp;
    
    INT32 nOutputBit;
    INT32 nOutputBitOld;
    
    UINT8* pEEPROMData;
} EEPROMDEV;

// External access to EEPROM state
extern EEPROMDEV EEPROMInfo;
extern UINT8* pEEPROMData;
extern INT32 nCaveWriteEEPROMCmd;

// To track if EEPROM is initialized
extern UINT8 DebugDev_EEPROMInitted;

// Proper implementation for EEPROM functions
extern "C" {
    void EEPROMInit() {
        DebugDev_EEPROMInitted = 1;
        
        // Clear EEPROM device structure
        memset(&EEPROMInfo, 0, sizeof(EEPROMDEV));
        
        // Set default values for EEPROM device
        EEPROMInfo.nDeviceType = 0;
        EEPROMInfo.nAddressBits = 8;
        EEPROMInfo.nDataBits = 8;
        EEPROMInfo.nSettings = 0;
        EEPROMInfo.nAddressMask = 0xFF;
        EEPROMInfo.nDataMask = 0xFF;
        
        EEPROMInfo.nCurrentAddress = 0;
        EEPROMInfo.nCurrentAddressMask = 1;
        EEPROMInfo.nReadAddress = 0;
        
        EEPROMInfo.nReadBit = 0;
        EEPROMInfo.nReadWriteBit = 0;
        EEPROMInfo.nClockBit = 0;
        EEPROMInfo.nEEPROMOp = 0;
        
        EEPROMInfo.nOutputBit = 0;
        EEPROMInfo.nOutputBitOld = 0;
        
        // Allocate EEPROM data memory
        EEPROMInfo.pEEPROMData = (UINT8*)BurnMalloc(512);
        pEEPROMData = EEPROMInfo.pEEPROMData;
        
        // Initialize memory to default state
        if (pEEPROMData) {
            memset(pEEPROMData, 0, 512);
        }
        
        nCaveWriteEEPROMCmd = 0;
    }
    
    void EEPROMExit() {
        if (!DebugDev_EEPROMInitted) return;
        
        // Free EEPROM data memory
        BurnFree(EEPROMInfo.pEEPROMData);
        pEEPROMData = NULL;
        
        // Reset init flag
        DebugDev_EEPROMInitted = 0;
    }
    
    void EEPROMReset() {
        if (!DebugDev_EEPROMInitted) return;
        
        // Reset EEPROM state
        EEPROMInfo.nCurrentAddress = 0;
        EEPROMInfo.nCurrentAddressMask = 1;
        EEPROMInfo.nReadAddress = 0;
        
        EEPROMInfo.nReadBit = 0;
        EEPROMInfo.nReadWriteBit = 0;
        EEPROMInfo.nClockBit = 0;
        EEPROMInfo.nEEPROMOp = 0;
        
        EEPROMInfo.nOutputBit = 0;
        EEPROMInfo.nOutputBitOld = 0;
    }
    
    INT32 EEPROMScan(INT32 nAction, INT32* pnMin) {
        if (!DebugDev_EEPROMInitted) return 0;
        
        struct BurnArea ba;
        
        if (nAction & ACB_DRIVER_DATA) {
            if (pnMin && *pnMin < 0x020902) {
                *pnMin = 0x020902;
            }
            
            ba.Data = &EEPROMInfo;
            ba.nLen = sizeof(EEPROMDEV);
            ba.nAddress = 0;
            ba.szName = "EEPROM Device";
            BurnAcb(&ba);
            
            if (pEEPROMData) {
                ba.Data = pEEPROMData;
                ba.nLen = 512;
                ba.nAddress = 0;
                ba.szName = "EEPROM Data";
                BurnAcb(&ba);
            }
        }
        
        return 0;
    }
} 