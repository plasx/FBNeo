#ifndef ROM_VERIFY_H
#define ROM_VERIFY_H

#include <stdbool.h>
#include "rom_verify_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Verify a ROM file exists and has the correct CRC32
bool ROM_Verify(const char* romPath);

// Verify CRC for Marvel vs Capcom ROM
bool VerifyCRCForMvsC(const char* zipPath);

// Dump contents of a ZIP file for debugging
int Metal_DumpZipContents(const char* zipPath);

// Calculate CRC32 for a block of data
UINT32 CalculateCRC32(const UINT8* data, UINT32 length);

// Verify a ROM in a ZIP file
bool VerifyZipROM(const char* zipPath, const char* romName, UINT32 expectedSize, UINT32 expectedCRC,
                  ROMVerificationResult* result);

#ifdef __cplusplus
}
#endif

#endif // ROM_VERIFY_H 