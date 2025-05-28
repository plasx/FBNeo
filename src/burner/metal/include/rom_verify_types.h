#ifndef ROM_VERIFY_TYPES_H
#define ROM_VERIFY_TYPES_H

#include <vector>
#include <string>

// Include standard types
#include "metal_declarations.h"

#ifdef __cplusplus
namespace ROMVerify {

// Structure to hold ROM checksum info
struct ROMChecksum {
    std::string romName;        // ROM filename
    std::string expectedCRC;    // Expected CRC32 (string format for easier comparison)
    std::string md5;            // MD5 hash (optional)
    std::string sha1;           // SHA1 hash (optional)
    bool isOptional;            // Is this ROM optional for the set?
};

// Structure for verification results of individual ROMs
struct VerificationResult {
    bool success;                 // Success/failure status
    std::string romName;          // ROM name
    std::string expectedChecksum; // Expected checksum
    std::string actualChecksum;   // Actual checksum
    std::string errorMessage;     // Error message if any
};

// Structure for verification results of ROM sets
struct ROMSetVerification {
    std::string setName;                     // ROM set name
    bool complete;                           // Is the set complete?
    bool playable;                           // Is the set playable despite missing optional ROMs?
    std::vector<VerificationResult> results; // Detailed results for individual ROMs
};

// Function declarations
bool VerifyROMSet(const char* romPath, ROMSetVerification& result);
bool VerifySingleROM(const char* romPath, VerificationResult& result);
bool CalculateROMChecksum(const char* romPath, std::string& crc, std::string& md5, std::string& sha1);
std::string GetChecksumDatabase();
bool LoadChecksumDatabase(const char* databasePath);
bool IsCPS2ROM(const char* romPath, bool deepScan = false);
bool VerifyCPS2ROM(const char* romPath, ROMSetVerification& result);

} // namespace ROMVerify

extern "C" {
#endif

// C-compatible verification result structure
typedef struct {
    int status;       // 0=success, 1=warning, -1=error
    char message[256]; // Status message
    UINT32 expectedCRC; // Expected CRC of ROM
    UINT32 actualCRC;  // Actual CRC of ROM
    UINT32 actualSize; // Actual size of ROM
    char romName[256]; // ROM name
} ROMVerificationResult;

// C interface function declarations
int Metal_VerifyGameROM(const char* gameName);
int Metal_DiagnoseROMLoading(const char* romPath);
bool VerifyCRCForMvsC(const char* zipPath);

#ifdef __cplusplus
}
#endif

#endif // ROM_VERIFY_TYPES_H 