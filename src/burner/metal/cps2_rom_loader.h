#pragma once

#include <string>
#include <vector>
#include <cstdint>

// CPS2 Hardware Type
enum CPS2HardwareType {
    CPS2_HW_STANDARD = 0,
    CPS2_HW_MARVEL,
    CPS2_HW_VAMPIRE,
    CPS2_HW_XMVSF,
    CPS2_HW_OTHER
};

// ROM file types
enum CPS2ROMType {
    CPS2_ROM_PROGRAM = 0,  // Program code (needs decryption)
    CPS2_ROM_GRAPHICS,     // Graphic data
    CPS2_ROM_SOUND,        // Audio data
    CPS2_ROM_QS,           // Q-Sound data
    CPS2_ROM_KEY,          // Encryption key
    CPS2_ROM_OTHER         // Other data
};

// Structure to represent a ROM file
struct CPS2ROMFile {
    std::string name;       // ROM filename
    size_t address;         // Load address
    size_t size;            // File size
    size_t maxSize;         // Maximum size
    uint32_t flags;         // Special flags
    bool optional;          // Whether the ROM is optional
    std::string checksum;   // Expected CRC32 checksum
    std::string md5;        // Expected MD5 checksum
    std::string sha1;       // Expected SHA1 checksum
    std::string path;       // Path to extracted file (filled at runtime)
    CPS2ROMType type;       // Type of ROM
    bool decrypted;         // Whether this ROM has been decrypted
    bool verified;          // Whether this ROM has been verified
    uint32_t actualCrc;     // Actual CRC32 (calculated at runtime)
};

// Structure to represent encryption options
struct CPS2Encryption {
    bool enabled;                // Whether encryption is used
    std::string key;             // Encryption key (hex)
    std::vector<std::string> keyFiles; // Key files in the ROM set
};

// Structure to represent a ROM set
struct CPS2ROMInfo {
    std::string id;                   // ROM set ID (e.g., "mvsc")
    std::string name;                 // Game name
    std::string parent;               // Parent ROM set ID for clones
    CPS2HardwareType hardwareType;    // Hardware type
    int width;                        // Display width
    int height;                       // Display height
    std::vector<CPS2ROMFile> files;   // ROM files
    CPS2Encryption encryption;        // Encryption information
    std::string year;                 // Release year
    std::string manufacturer;         // Manufacturer
    bool bios;                        // Whether this is a BIOS ROM set
    std::string region;               // Region code (USA, Japan, etc.)
    std::string version;              // Version/revision
};

// Structure for a CPS2 game entry
struct CPS2GameInfo {
    std::string id;             // Game ID
    std::string name;           // Game name
    CPS2ROMInfo romInfo;        // ROM information
    bool romAvailable;          // Whether the ROM is available
    std::string status;         // Status (working, not working, etc.)
    std::string category;       // Category (fighting, etc.)
    std::string description;    // Long description
};

// Initialize the CPS2 ROM loader
bool CPS2_InitROMLoader();

// Shutdown the CPS2 ROM loader
void CPS2_ShutdownROMLoader();

// Load a CPS2 ROM set by game ID
bool CPS2_LoadROMSet(const char* gameId);

// Get information about the currently loaded ROM set
const CPS2ROMInfo* CPS2_GetROMInfo();

// Get a specific ROM file by name
const CPS2ROMFile* CPS2_GetROMFile(const char* name);

// Load a CPS2 ROM file into memory
bool CPS2_LoadROMData(const char* name, void* buffer, size_t size);

// Clean up temporary files
void CPS2_CleanupROMFiles();

// Run a loaded CPS2 ROM
bool CPS2_RunROM();

// Get a list of supported CPS2 games
std::vector<CPS2GameInfo> CPS2_GetSupportedGames();

// Connect to FBNeo core
void CPS2_SetupMetalLinkage();

// Add a new ROM to the database
bool CPS2_AddROMToDatabase(const CPS2ROMInfo& romInfo);

// Check if a specific game is supported
bool CPS2_IsGameSupported(const char* gameId);

// Get memory mapping for a specific game (used for gameplay and AI)
bool CPS2_GetMemoryMap(const char* gameId, void* mapData, size_t mapSize);

// Verify the loaded ROM set
bool CPS2_VerifyLoadedROM(); 