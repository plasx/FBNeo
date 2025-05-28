#include "burner.h"
#include "burnint.h"
#include "../metal_declarations.h"
#include "../metal_bridge.h"
#include "../rom_verify.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <string>

// Test program to verify ROM functionality
int main(int argc, char* argv[]) {
    printf("ROM Verification Test Program\n");
    printf("============================\n\n");
    
    // Check if a ROM path was provided
    if (argc < 2) {
        printf("Usage: %s <rom_path>\n", argv[0]);
        printf("Example: %s /path/to/roms/mvsc.zip\n", argv[0]);
        return 1;
    }
    
    const char* romPath = argv[1];
    printf("Testing ROM verification for: %s\n", romPath);
    
    // 1. Check if it's a CPS2 ROM
    bool isCps2 = ROMVerify::IsCPS2ROM(romPath, true);
    printf("\nCPS2 ROM Detection:\n");
    printf("  Is CPS2 ROM: %s\n", isCps2 ? "YES" : "NO");
    
    // 2. Get ROM checksum
    std::string crc, md5, sha1;
    bool checksumResult = ROMVerify::CalculateROMChecksum(romPath, crc, md5, sha1);
    
    printf("\nROM Checksum Calculation:\n");
    if (checksumResult) {
        printf("  CRC32: %s\n", crc.c_str());
        printf("  MD5: %s\n", md5.c_str());
        printf("  SHA1: %s\n", sha1.c_str());
    } else {
        printf("  Failed to calculate checksums\n");
    }
    
    // 3. Verify single ROM
    ROMVerify::VerificationResult singleResult;
    bool singleVerified = ROMVerify::VerifySingleROM(romPath, singleResult);
    
    printf("\nSingle ROM Verification:\n");
    printf("  ROM Name: %s\n", singleResult.romName.c_str());
    printf("  Success: %s\n", singleResult.success ? "YES" : "NO");
    printf("  Actual Checksum: %s\n", singleResult.actualChecksum.c_str());
    printf("  Expected Checksum: %s\n", singleResult.expectedChecksum.c_str());
    printf("  Message: %s\n", singleResult.errorMessage.c_str());
    
    // 4. Verify full ROM set
    ROMVerify::ROMSetVerification setResult;
    bool setVerified = ROMVerify::VerifyROMSet(romPath, setResult);
    
    printf("\nROM Set Verification:\n");
    printf("  Set Name: %s\n", setResult.setName.c_str());
    printf("  Complete: %s\n", setResult.complete ? "YES" : "NO");
    printf("  Playable: %s\n", setResult.playable ? "YES" : "NO");
    printf("  Results:\n");
    
    for (const auto& result : setResult.results) {
        printf("    - %s: %s\n", result.romName.c_str(), result.success ? "OK" : "FAILED");
        if (!result.success) {
            printf("      Error: %s\n", result.errorMessage.c_str());
            if (!result.actualChecksum.empty() && !result.expectedChecksum.empty()) {
                printf("      Expected: %s, Actual: %s\n", 
                      result.expectedChecksum.c_str(), 
                      result.actualChecksum.c_str());
            }
        }
    }
    
    // 5. Test CPS2-specific verification
    printf("\nCPS2-Specific Verification:\n");
    ROMVerify::ROMSetVerification cps2Result;
    bool cps2Verified = ROMVerify::VerifyCPS2ROM(romPath, cps2Result);
    
    if (isCps2) {
        printf("  CPS2 Verification Result: %s\n", cps2Verified ? "SUCCESS" : "FAILED");
        printf("  Playable: %s\n", cps2Result.playable ? "YES" : "NO");
    } else {
        printf("  Not a CPS2 ROM, verification skipped\n");
    }
    
    // 6. Print checksum database info
    printf("\nChecksum Database Summary:\n");
    std::string dbInfo = ROMVerify::GetChecksumDatabase();
    printf("%s\n", dbInfo.c_str());
    
    return 0;
} 