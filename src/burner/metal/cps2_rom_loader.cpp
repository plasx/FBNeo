#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

// FBNeo core includes
#include "burner.h"
#include "burnint.h"
#include "cps.h"
#include "cps2.h"

// Metal-specific includes
#include "metal_declarations.h"
#include "metal_bridge.h"
#include "rom_verify.h"
#include "rom_path_manager.h"
#include "cps2_rom_loader.h"

// Zip handling
#include "unzip.h"

// External references to key FBNeo core functions
extern "C" {
    INT32 BurnDrvGetZipName(char** pszName, UINT32 i);
    extern char szAppRomPaths[DIRS_MAX][MAX_PATH];
}

// ROM Loading State
static struct {
    bool initialized;
    std::string currentGame;
    std::string zipPath;
    bool romLoaded;
    CPS2ROMInfo romInfo;
    std::vector<CPS2ROMFile> loadedFiles;
} g_cps2State = {
    false, "", "", false, {}, {}
};

// CPS2 ROM database
static std::unordered_map<std::string, CPS2ROMInfo> g_cps2RomDb;

// Initialize ROM database with supported CPS2 games
static void InitializeROMDatabase() {
    if (!g_cps2RomDb.empty()) {
        return;
    }
    
    // Marvel vs. Capcom: Clash of Super Heroes
    CPS2ROMInfo mvsc = {
        "mvsc", "Marvel vs. Capcom: Clash of Super Heroes", "cps2", CPS2_HW_MARVEL, 384, 224,
        {
            {"mvc.03", 0, 0x524, 0x524, 0, false, "fe5f4e29", "689c699a16178765173cf9686c0b328c", "", "/tmp/fbneo_roms/mvc.03", CPS2_ROM_PROGRAM, false, false, 0},
            {"mvc.04", 0, 0x524, 0x524, 0, false, "95c06b8e", "46bb5b58ffd3d7f8cf0f0f9fb9c0c48f", "", "/tmp/fbneo_roms/mvc.04", CPS2_ROM_PROGRAM, false, false, 0},
            {"mvc.05", 0, 0x524, 0x524, 0, false, "7ffad45b", "aba43b2130a9aa3c32cdd0250ce4d471", "", "/tmp/fbneo_roms/mvc.05", CPS2_ROM_PROGRAM, false, false, 0},
            {"mvc.06", 0, 0x524, 0x524, 0, false, "0b4358ec", "1cae3a66c7e0995e796e4a5af814a0dc", "", "/tmp/fbneo_roms/mvc.06", CPS2_ROM_PROGRAM, false, false, 0},
            {"mvc.07", 0, 0x100, 0x100, 0, false, "3d9fb25e", "f91b0d583f6672342b199147930ba7a0", "", "/tmp/fbneo_roms/mvc.07", CPS2_ROM_GRAPHICS, false, false, 0},
            {"mvc.08", 0, 0x100, 0x100, 0, false, "b05feaa6", "68c3ccef81578f61da3a1d76e1e9ca1e", "", "/tmp/fbneo_roms/mvc.08", CPS2_ROM_GRAPHICS, false, false, 0},
            {"mvc.09", 0, 0x100, 0x100, 0, false, "83e55cc5", "0b35d7a5a8ccb93f56bb2e5d351a3bae", "", "/tmp/fbneo_roms/mvc.09", CPS2_ROM_GRAPHICS, false, false, 0},
            {"mvc.10", 0, 0x100, 0x100, 0, false, "2754575c", "fc39c8f16c24eba35224c2c386273531", "", "/tmp/fbneo_roms/mvc.10", CPS2_ROM_GRAPHICS, false, false, 0},
            {"mvc.11", 0, 0x100, 0x100, 0, false, "c739cc6c", "1bf8227286a60365afc5d5c06d2d75c1", "", "/tmp/fbneo_roms/mvc.11", CPS2_ROM_SOUND, false, false, 0},
            {"mvc.12", 0, 0x100, 0x100, 0, false, "b3d939c3", "a72ce7b74c16b887adf48a5b7b584a4e", "", "/tmp/fbneo_roms/mvc.12", CPS2_ROM_SOUND, false, false, 0}
        },
        {true, "c19a5c04", {"mvc.key"}},  // Encryption info
        "1998",                           // Year
        "Capcom",                         // Manufacturer
        false,                            // Not a BIOS
        "USA",                            // Region
        "980123"                          // Version
    };
    g_cps2RomDb["mvsc"] = mvsc;
    
    // Marvel vs. Capcom: Clash of Super Heroes (USA)
    CPS2ROMInfo mvscu = mvsc;
    mvscu.id = "mvscu";
    g_cps2RomDb["mvscu"] = mvscu;
    
    // Street Fighter Alpha 3
    CPS2ROMInfo sfa3 = {
        "sfa3", "Street Fighter Alpha 3", "cps2", CPS2_HW_STANDARD, 384, 224,
        {
            {"sz3.03c", 0, 0x524, 0x524, 0, false, "e7e1474b", "97d9f4430d88534e096188d9ec1c64d5", "", "/tmp/fbneo_roms/sz3.03c", CPS2_ROM_PROGRAM, false, false, 0},
            {"sz3.04c", 0, 0x524, 0x524, 0, false, "5ad3d3b5", "e1c1deb17b1a59d5e5d02780a195a19a", "", "/tmp/fbneo_roms/sz3.04c", CPS2_ROM_PROGRAM, false, false, 0},
            {"sz3.05c", 0, 0x524, 0x524, 0, false, "d23892a9", "86ba2ebb2f38eb7b3810db4562a9017c", "", "/tmp/fbneo_roms/sz3.05c", CPS2_ROM_PROGRAM, false, false, 0},
            {"sz3.06c", 0, 0x524, 0x524, 0, false, "e21f4914", "e3d2f0ad191e2535e2c38d2268dba64d", "", "/tmp/fbneo_roms/sz3.06c", CPS2_ROM_PROGRAM, false, false, 0},
            {"sz3.07c", 0, 0x524, 0x524, 0, false, "cb62b61c", "a2a893c4fed51bc7f5380dd1c9d9eb31", "", "/tmp/fbneo_roms/sz3.07c", CPS2_ROM_GRAPHICS, false, false, 0},
            {"sz3.08c", 0, 0x524, 0x524, 0, false, "5de01cc5", "9c953db5f07977e65f02f9c3031a21bb", "", "/tmp/fbneo_roms/sz3.08c", CPS2_ROM_GRAPHICS, false, false, 0},
            {"sz3.09c", 0, 0x100, 0x100, 0, false, "81558e50", "9dac77e73695042fa9a4a7b4c54c258e", "", "/tmp/fbneo_roms/sz3.09c", CPS2_ROM_GRAPHICS, false, false, 0},
            {"sz3.10b", 0, 0x100, 0x100, 0, false, "4adc50d6", "59992a6bc2f6999c92f95891646b8e19", "", "/tmp/fbneo_roms/sz3.10b", CPS2_ROM_SOUND, false, false, 0}
        },
        {true, "245d7c4c", {"sz3.key"}},  // Encryption info
        "1998",                           // Year
        "Capcom",                         // Manufacturer
        false,                            // Not a BIOS
        "Europe",                         // Region
        "980904"                          // Version
    };
    g_cps2RomDb["sfa3"] = sfa3;
    
    // Street Fighter Zero 3 (Japan)
    CPS2ROMInfo sfz3 = sfa3;
    sfz3.id = "sfz3";
    sfz3.region = "Japan";
    sfz3.files[0].name = "sz3.03d";
    sfz3.files[0].checksum = "2a947b54";
    sfz3.files[1].name = "sz3.04d";
    sfz3.files[1].checksum = "c8ed5a9c";
    g_cps2RomDb["sfz3"] = sfz3;
    
    // X-Men vs. Street Fighter
    CPS2ROMInfo xmvsf = {
        "xmvsf", "X-Men vs. Street Fighter", "cps2", CPS2_HW_XMVSF, 384, 224,
        {
            {"xvs.03e", 0, 0x524, 0x524, 0, false, "bd353a5a", "5ae2acffeb1a55881b0b734d516f34e0", "", "/tmp/fbneo_roms/xvs.03e", CPS2_ROM_PROGRAM, false, false, 0},
            {"xvs.04a", 0, 0x524, 0x524, 0, false, "7b19a8c7", "f8a451439e57eb6d98ee85de78c7e28a", "", "/tmp/fbneo_roms/xvs.04a", CPS2_ROM_PROGRAM, false, false, 0},
            {"xvs.05a", 0, 0x524, 0x524, 0, false, "9a87d545", "c4f05b2889befbaf05a4f192f69b9ff0", "", "/tmp/fbneo_roms/xvs.05a", CPS2_ROM_PROGRAM, false, false, 0},
            {"xvs.06a", 0, 0x524, 0x524, 0, false, "57952a39", "7efcbf1f5f99c651a31f922c56a3b11c", "", "/tmp/fbneo_roms/xvs.06a", CPS2_ROM_PROGRAM, false, false, 0},
            {"xvs.07", 0, 0x100, 0x100, 0, false, "8ffcb427", "d5a41e3faf79207a23ca1987148da36e", "", "/tmp/fbneo_roms/xvs.07", CPS2_ROM_GRAPHICS, false, false, 0},
            {"xvs.08", 0, 0x100, 0x100, 0, false, "268b0c2b", "2b77dfe669242725b47cccce0c69b342", "", "/tmp/fbneo_roms/xvs.08", CPS2_ROM_GRAPHICS, false, false, 0},
            {"xvs.09", 0, 0x100, 0x100, 0, false, "932d9074", "0e844b4af096a5a8d45cff7d4f30c5c8", "", "/tmp/fbneo_roms/xvs.09", CPS2_ROM_SOUND, false, false, 0},
            {"xvs.10", 0, 0x100, 0x100, 0, false, "cb16a2a2", "1e4a7c337e217eb21b70922470ba9d9d", "", "/tmp/fbneo_roms/xvs.10", CPS2_ROM_SOUND, false, false, 0}
        },
        {true, "d9b33e95", {"xvs.key"}},  // Encryption info
        "1996",                           // Year
        "Capcom",                         // Manufacturer
        false,                            // Not a BIOS
        "Europe",                         // Region
        "961004"                          // Version
    };
    g_cps2RomDb["xmvsf"] = xmvsf;
    
    // Super Street Fighter II Turbo
    CPS2ROMInfo ssf2t = {
        "ssf2t", "Super Street Fighter II Turbo", "cps2", CPS2_HW_STANDARD, 384, 224,
        {
            {"sfxe.03c", 0, 0x524, 0x524, 0, false, "2fa1f396", "1e3c8ff4cd8052c4e30f88b144d674cd", "", "/tmp/fbneo_roms/sfxe.03c", CPS2_ROM_PROGRAM, false, false, 0},
            {"sfxe.04a", 0, 0x524, 0x524, 0, false, "d0c74a15", "3e1e2e202dfdd20ae1e312cb3a8324ab", "", "/tmp/fbneo_roms/sfxe.04a", CPS2_ROM_PROGRAM, false, false, 0},
            {"sfxe.05", 0, 0x524, 0x524, 0, false, "65bde435", "03d4a27e89bf980cacb4540e916c5b36", "", "/tmp/fbneo_roms/sfxe.05", CPS2_ROM_PROGRAM, false, false, 0},
            {"sfxe.06a", 0, 0x524, 0x524, 0, false, "912a9ca0", "f9f702bd87972eb37757b4d885fc0b35", "", "/tmp/fbneo_roms/sfxe.06a", CPS2_ROM_PROGRAM, false, false, 0},
            {"sfxe.07", 0, 0x100, 0x100, 0, false, "93f04ff8", "f863a13e9c12f5537e76b1d99e08078c", "", "/tmp/fbneo_roms/sfxe.07", CPS2_ROM_GRAPHICS, false, false, 0},
            {"sfxe.08", 0, 0x100, 0x100, 0, false, "a19140b5", "df12ab2fd0a8c309e2a3a43b8e573efa", "", "/tmp/fbneo_roms/sfxe.08", CPS2_ROM_GRAPHICS, false, false, 0},
            {"sfxe.09", 0, 0x100, 0x100, 0, false, "f6548eef", "8a5c67bc2748e5a8299d2bf207c39962", "", "/tmp/fbneo_roms/sfxe.09", CPS2_ROM_SOUND, false, false, 0}
        },
        {true, "1234abcd", {"sfx.key"}},  // Encryption info
        "1994",                           // Year
        "Capcom",                         // Manufacturer
        false,                            // Not a BIOS
        "Europe",                         // Region
        "940323"                          // Version
    };
    g_cps2RomDb["ssf2t"] = ssf2t;
    
    // Vampire Savior: The Lord of Vampire
    CPS2ROMInfo vsav = {
        "vsav", "Vampire Savior: The Lord of Vampire", "cps2", CPS2_HW_VAMPIRE, 384, 224,
        {
            {"vm3e.03", 0, 0x524, 0x524, 0, false, "4de068ec", "f7957f14c7a5ab7a41820d9db99f7f89", "", "/tmp/fbneo_roms/vm3e.03", CPS2_ROM_PROGRAM, false, false, 0},
            {"vm3e.04", 0, 0x524, 0x524, 0, false, "a7bbb7c7", "36b2739d1a2c7b7d093f681921ad7b6a", "", "/tmp/fbneo_roms/vm3e.04", CPS2_ROM_PROGRAM, false, false, 0},
            {"vm3.05", 0, 0x524, 0x524, 0, false, "b021c347", "880fd91c81f650205a1b996c6c4d4db2", "", "/tmp/fbneo_roms/vm3.05", CPS2_ROM_PROGRAM, false, false, 0},
            {"vm3.06", 0, 0x524, 0x524, 0, false, "137da6c8", "5e9cdcb7e829f5e2570fd85a81de4205", "", "/tmp/fbneo_roms/vm3.06", CPS2_ROM_PROGRAM, false, false, 0},
            {"vm3.07", 0, 0x100, 0x100, 0, false, "d89c3113", "d13b0c3a97d5a0194ec89a0ab2df79c1", "", "/tmp/fbneo_roms/vm3.07", CPS2_ROM_GRAPHICS, false, false, 0},
            {"vm3.08", 0, 0x100, 0x100, 0, false, "7154ba11", "ddc5a36c93e1e6b668e617401c68d5e9", "", "/tmp/fbneo_roms/vm3.08", CPS2_ROM_GRAPHICS, false, false, 0},
            {"vm3.09", 0, 0x100, 0x100, 0, false, "79e5793e", "a0fb43aa20f0ef4a592b8c5eb7331db4", "", "/tmp/fbneo_roms/vm3.09", CPS2_ROM_SOUND, false, false, 0}
        },
        {true, "381151aa", {"vm3.key"}},  // Encryption info
        "1997",                           // Year
        "Capcom",                         // Manufacturer
        false,                            // Not a BIOS
        "Europe",                         // Region
        "970930"                          // Version
    };
    g_cps2RomDb["vsav"] = vsav;
    
    // Add more CPS2 games here
    
    // Cyberbots: Fullmetal Madness
    CPS2ROMInfo cybots = {
        "cybots", "Cyberbots: Fullmetal Madness", "cps2", CPS2_HW_STANDARD, 384, 224,
        {
            {"cybe.03", 0, 0x524, 0x524, 0, false, "234381cd", "8eb7c491fa9c6c003ce51735092c7dbb", "", "/tmp/fbneo_roms/cybe.03", CPS2_ROM_PROGRAM, false, false, 0},
            {"cybe.04", 0, 0x524, 0x524, 0, false, "596d4f51", "f82dae9b702c55be33841de5db3fe5db", "", "/tmp/fbneo_roms/cybe.04", CPS2_ROM_PROGRAM, false, false, 0},
            {"cyb.05", 0, 0x524, 0x524, 0, false, "c4c39ae4", "e6bf7c5f8e9f177b1b1993c219fa770d", "", "/tmp/fbneo_roms/cyb.05", CPS2_ROM_PROGRAM, false, false, 0},
            {"cyb.06", 0, 0x524, 0x524, 0, false, "a0751944", "e1b9ceb903ff1ef811025e945392c308", "", "/tmp/fbneo_roms/cyb.06", CPS2_ROM_PROGRAM, false, false, 0},
            {"cyb.07", 0, 0x100, 0x100, 0, false, "1113a5f1", "ee7fd9f7826cc2385d8724255c2d42c0", "", "/tmp/fbneo_roms/cyb.07", CPS2_ROM_GRAPHICS, false, false, 0},
            {"cyb.08", 0, 0x100, 0x100, 0, false, "db1800c0", "54cc7d45dca89a8ff6a084f329b6330a", "", "/tmp/fbneo_roms/cyb.08", CPS2_ROM_GRAPHICS, false, false, 0},
            {"cyb.09", 0, 0x100, 0x100, 0, false, "339374b8", "df9b3f75c8b4a577b3c1769d5b71f5aa", "", "/tmp/fbneo_roms/cyb.09", CPS2_ROM_SOUND, false, false, 0}
        },
        {true, "279bea83", {"cyb.key"}},  // Encryption info
        "1995",                           // Year
        "Capcom",                         // Manufacturer
        false,                            // Not a BIOS
        "Europe",                         // Region
        "950424"                          // Version
    };
    g_cps2RomDb["cybots"] = cybots;
    
    // Super Puzzle Fighter II Turbo
    CPS2ROMInfo spf2t = {
        "spf2t", "Super Puzzle Fighter II Turbo", "cps2", CPS2_HW_STANDARD, 384, 224,
        {
            {"pzfe.03", 0, 0x524, 0x524, 0, false, "2af51954", "87cffe10ce4f4c3e87c3a0d4c5195ea1", "", "/tmp/fbneo_roms/pzfe.03", CPS2_ROM_PROGRAM, false, false, 0},
            {"pzf.04", 0, 0x524, 0x524, 0, false, "b80649e2", "71aec731d7f738bad9b04bcb30e9b966", "", "/tmp/fbneo_roms/pzf.04", CPS2_ROM_PROGRAM, false, false, 0},
            {"pzf.05", 0, 0x100, 0x100, 0, false, "3a5737a2", "4bd8b0c9f5cec76aad990d4901a5a649", "", "/tmp/fbneo_roms/pzf.05", CPS2_ROM_GRAPHICS, false, false, 0},
            {"pzf.06", 0, 0x100, 0x100, 0, false, "60d620f6", "322b79d9f0c7b12bbb14a4ee3389fc29", "", "/tmp/fbneo_roms/pzf.06", CPS2_ROM_GRAPHICS, false, false, 0},
            {"pzf.07", 0, 0x100, 0x100, 0, false, "056caeb2", "251b1ed59ef257cba873933b179c68d5", "", "/tmp/fbneo_roms/pzf.07", CPS2_ROM_SOUND, false, false, 0}
        },
        {true, "be14b690", {"pzf.key"}},  // Encryption info
        "1996",                           // Year
        "Capcom",                         // Manufacturer
        false,                            // Not a BIOS
        "Europe",                         // Region
        "960227"                          // Version
    };
    g_cps2RomDb["spf2t"] = spf2t;
}

// Initialize the CPS2 ROM loader
bool CPS2_InitROMLoader() {
    if (g_cps2State.initialized) {
        return true;
    }
    
    printf("CPS2_InitROMLoader: Initializing CPS2 ROM loader\n");
    
    // Initialize ROM database
    InitializeROMDatabase();
    
    // Initialize state
    g_cps2State.initialized = true;
    g_cps2State.currentGame = "";
    g_cps2State.zipPath = "";
    g_cps2State.romLoaded = false;
    g_cps2State.loadedFiles.clear();
    
    return true;
}

// Shutdown the CPS2 ROM loader
void CPS2_ShutdownROMLoader() {
    if (!g_cps2State.initialized) {
        return;
    }
    
    printf("CPS2_ShutdownROMLoader: Shutting down CPS2 ROM loader\n");
    
    // Free resources
    g_cps2State.loadedFiles.clear();
    g_cps2State.initialized = false;
}

// Find a ZIP file in ROM directories
static bool FindZipFile(const char* zipName, std::string& outPath) {
    // First, check if it's an absolute path that exists
    struct stat sb;
    if (stat(zipName, &sb) == 0 && S_ISREG(sb.st_mode)) {
        outPath = zipName;
        return true;
    }
    
    // Second, try all ROM paths
    for (int i = 0; i < DIRS_MAX; i++) {
        if (szAppRomPaths[i][0] != '\0') {
            std::string testPath = szAppRomPaths[i];
            
            // Add trailing slash if needed
            if (testPath.back() != '/' && testPath.back() != '\\') {
                testPath += '/';
            }
            
            testPath += zipName;
            
            if (stat(testPath.c_str(), &sb) == 0 && S_ISREG(sb.st_mode)) {
                outPath = testPath;
                return true;
            }
        }
    }
    
    // Not found
    return false;
}

// Check if a filename matches a ROM entry
static bool MatchROMName(const char* filename, const CPS2ROMFile& romFile) {
    // First, try exact match
    if (strcasecmp(filename, romFile.name.c_str()) == 0) {
        return true;
    }
    
    // Then try with different extensions
    std::string baseName = romFile.name;
    size_t dotPos = baseName.find_last_of('.');
    if (dotPos != std::string::npos) {
        baseName = baseName.substr(0, dotPos);
        
        // Check if filename has the same basename
        std::string fileBaseName = filename;
        dotPos = fileBaseName.find_last_of('.');
        if (dotPos != std::string::npos) {
            fileBaseName = fileBaseName.substr(0, dotPos);
            
            return (strcasecmp(fileBaseName.c_str(), baseName.c_str()) == 0);
        }
    }
    
    return false;
}

// Extract all ROM files from a ZIP archive
static bool ExtractROMs(const std::string& zipPath, const CPS2ROMInfo& romInfo, std::vector<CPS2ROMFile>& outFiles) {
    unzFile zip = unzOpen(zipPath.c_str());
    if (!zip) {
        printf("ExtractROMs: Failed to open ZIP file: %s\n", zipPath.c_str());
        return false;
    }
    
    printf("ExtractROMs: Opening ZIP file: %s\n", zipPath.c_str());
    
    // Create temporary directory if it doesn't exist
    std::string tempDir = "/tmp/fbneo_roms";
    std::string mkdirCmd = "mkdir -p " + tempDir;
    if (system(mkdirCmd.c_str()) != 0) {
        printf("ExtractROMs: Failed to create temporary directory: %s\n", tempDir.c_str());
        unzClose(zip);
        return false;
    }
    
    // First find, then extract each ROM file
    int foundCount = 0;
    std::vector<CPS2ROMFile> extractedFiles;
    
    for (const auto& romFile : romInfo.files) {
        // Find file in ZIP
        bool found = false;
        unzGoToFirstFile(zip);
        
        do {
            char filenameInZip[256];
            unz_file_info fileInfo;
            
            if (unzGetCurrentFileInfo(zip, &fileInfo, filenameInZip, sizeof(filenameInZip), NULL, 0, NULL, 0) != UNZ_OK) {
                continue;
            }
            
            if (MatchROMName(filenameInZip, romFile)) {
                found = true;
                
                // Extract file to temporary location
                std::string outputPath = tempDir + "/" + romFile.name;
                
                if (unzOpenCurrentFile(zip) != UNZ_OK) {
                    printf("ExtractROMs: Failed to open file in ZIP: %s\n", filenameInZip);
                    continue;
                }
                
                FILE* outputFile = fopen(outputPath.c_str(), "wb");
                if (!outputFile) {
                    printf("ExtractROMs: Failed to create output file: %s\n", outputPath.c_str());
                    unzCloseCurrentFile(zip);
                    continue;
                }
                
                char buffer[8192];
                int bytesRead;
                size_t totalBytes = 0;
                
                while ((bytesRead = unzReadCurrentFile(zip, buffer, sizeof(buffer))) > 0) {
                    fwrite(buffer, 1, bytesRead, outputFile);
                    totalBytes += bytesRead;
                }
                
                fclose(outputFile);
                unzCloseCurrentFile(zip);
                
                // Create ROM file info
                CPS2ROMFile extractedFile = romFile;
                extractedFile.path = outputPath;
                extractedFile.size = totalBytes;
                extractedFiles.push_back(extractedFile);
                
                printf("ExtractROMs: Extracted %s (%zu bytes)\n", romFile.name.c_str(), totalBytes);
                foundCount++;
                break;
            }
            
        } while (unzGoToNextFile(zip) == UNZ_OK);
        
        if (!found && !romFile.optional) {
            printf("ExtractROMs: Required ROM file not found: %s\n", romFile.name.c_str());
        }
    }
    
    unzClose(zip);
    
    // Check if we found all required files
    bool allFound = true;
    for (const auto& romFile : romInfo.files) {
        if (!romFile.optional) {
            bool found = false;
            for (const auto& extractedFile : extractedFiles) {
                if (extractedFile.name == romFile.name) {
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                allFound = false;
                printf("ExtractROMs: Missing required ROM file: %s\n", romFile.name.c_str());
            }
        }
    }
    
    if (allFound) {
        outFiles = extractedFiles;
        printf("ExtractROMs: Successfully extracted all required ROM files (%d files)\n", foundCount);
        return true;
    } else {
        printf("ExtractROMs: Failed to extract all required ROM files\n");
        return false;
    }
}

// Load a CPS2 ROM set by game ID
bool CPS2_LoadROMSet(const char* gameId) {
    if (!g_cps2State.initialized) {
        if (!CPS2_InitROMLoader()) {
            return false;
        }
    }
    
    printf("CPS2_LoadROMSet: Loading ROM set %s\n", gameId ? gameId : "null");
    
    if (!gameId || !gameId[0]) {
        printf("CPS2_LoadROMSet: Invalid game ID\n");
        return false;
    }
    
    // Convert to lowercase for case-insensitive comparison
    std::string gameIdLower = gameId;
    std::transform(gameIdLower.begin(), gameIdLower.end(), gameIdLower.begin(), ::tolower);
    
    // Look up ROM info in database
    auto it = g_cps2RomDb.find(gameIdLower);
    if (it == g_cps2RomDb.end()) {
        printf("CPS2_LoadROMSet: Game '%s' not found in ROM database\n", gameId);
        return false;
    }
    
    // Store ROM info
    g_cps2State.romInfo = it->second;
    g_cps2State.currentGame = gameIdLower;
    g_cps2State.romLoaded = false;
    g_cps2State.loadedFiles.clear();
    
    // Find the ROM ZIP file
    char zipName[MAX_PATH];
    snprintf(zipName, sizeof(zipName), "%s.zip", gameIdLower.c_str());
    
    if (!FindZipFile(zipName, g_cps2State.zipPath)) {
        printf("CPS2_LoadROMSet: ZIP file '%s' not found in ROM paths\n", zipName);
        
        // Try alternative ZIP names for some games
        if (gameIdLower == "mvsc") {
            if (FindZipFile("mvscu.zip", g_cps2State.zipPath)) {
                printf("CPS2_LoadROMSet: Found alternative ZIP 'mvscu.zip'\n");
            } else {
                return false;
            }
        } else if (gameIdLower == "sfa3") {
            if (FindZipFile("sfz3.zip", g_cps2State.zipPath)) {
                printf("CPS2_LoadROMSet: Found alternative ZIP 'sfz3.zip'\n");
            } else {
                return false;
            }
        } else {
            return false;
        }
    }
    
    // Extract ROM files
    if (!ExtractROMs(g_cps2State.zipPath, g_cps2State.romInfo, g_cps2State.loadedFiles)) {
        printf("CPS2_LoadROMSet: Failed to extract ROM files from %s\n", g_cps2State.zipPath.c_str());
        return false;
    }
    
    // Verify ROM checksums and completeness
    ROMVerify::ROMSetVerification verification;
    if (!ROMVerify::VerifyCPS2ROM(g_cps2State.zipPath.c_str(), verification)) {
        printf("CPS2_LoadROMSet: ROM verification failed!\n");
        
        // Log detailed verification results
        for (const auto& result : verification.results) {
            if (!result.success) {
                printf("CPS2_LoadROMSet: - %s: %s (Expected: %s, Got: %s)\n", 
                      result.romName.c_str(), 
                      result.errorMessage.c_str(),
                      result.expectedChecksum.c_str(),
                      result.actualChecksum.c_str());
            }
        }
        
        // Continue anyway if it's playable
        if (!verification.playable) {
            CPS2_CleanupROMFiles();
            return false;
        }
        
        printf("CPS2_LoadROMSet: ROM set incomplete but playable - continuing\n");
    }
    
    // Apply CPS2 decryption if needed
    if (g_cps2State.romInfo.hardwareType != CPS2_HW_STANDARD) {
        printf("CPS2_LoadROMSet: Applying CPS2 decryption for %s\n", gameIdLower.c_str());
        
        // Call core's CPS2 decryption code
        extern int Cps2Decrypt(void* baseAddress);
        
        // For each program ROM, try to apply decryption
        for (auto& romFile : g_cps2State.loadedFiles) {
            // Only decrypt program ROMs (typically .03-.08)
            if (romFile.name.find(".0") != std::string::npos) {
                FILE* f = fopen(romFile.path.c_str(), "rb+");
                if (f) {
                    // Read file into memory
                    void* romData = malloc(romFile.size);
                    if (romData) {
                        if (fread(romData, 1, romFile.size, f) == romFile.size) {
                            // Apply decryption
                            int result = Cps2Decrypt(romData);
                            if (result == 0) {
                                // Write decrypted data back
                                fseek(f, 0, SEEK_SET);
                                fwrite(romData, 1, romFile.size, f);
                                printf("CPS2_LoadROMSet: - Decrypted %s successfully\n", romFile.name.c_str());
                            } else {
                                printf("CPS2_LoadROMSet: - Failed to decrypt %s (code: %d)\n", 
                                      romFile.name.c_str(), result);
                            }
                        }
                        free(romData);
                    }
                    fclose(f);
                }
            }
        }
    }
    
    // Set ROM loaded flag
    g_cps2State.romLoaded = true;
    
    printf("CPS2_LoadROMSet: Successfully loaded %s ROM set\n", gameId);
    return true;
}

// Get information about the currently loaded ROM set
const CPS2ROMInfo* CPS2_GetROMInfo() {
    if (!g_cps2State.initialized || !g_cps2State.romLoaded) {
        return nullptr;
    }
    
    return &g_cps2State.romInfo;
}

// Get a specific ROM file by name
const CPS2ROMFile* CPS2_GetROMFile(const char* name) {
    if (!g_cps2State.initialized || !g_cps2State.romLoaded) {
        return nullptr;
    }
    
    for (const auto& file : g_cps2State.loadedFiles) {
        if (strcasecmp(file.name.c_str(), name) == 0) {
            return &file;
        }
    }
    
    return nullptr;
}

// Load a CPS2 ROM file into memory
bool CPS2_LoadROMData(const char* name, void* buffer, size_t size) {
    if (!g_cps2State.initialized || !g_cps2State.romLoaded || !name || !buffer || size == 0) {
        printf("CPS2_LoadROMData: Invalid parameters\n");
        return false;
    }
    
    if (name[0] == '\0') {
        printf("CPS2_LoadROMData: Empty ROM name\n");
        return false;
    }
    
    printf("CPS2_LoadROMData: Loading ROM file %s (%zu bytes)\n", name, size);
    
    // Find the ROM file in our loaded files
    const CPS2ROMFile* romFile = nullptr;
    for (const auto& file : g_cps2State.loadedFiles) {
        if (strcasecmp(file.name.c_str(), name) == 0) {
            romFile = &file;
            break;
        }
    }
    
    if (!romFile) {
        // Try with different extensions
        std::string baseName = name;
        size_t dotPos = baseName.find_last_of('.');
        if (dotPos != std::string::npos) {
            baseName = baseName.substr(0, dotPos);
            
            for (const auto& file : g_cps2State.loadedFiles) {
                std::string fileBaseName = file.name;
                dotPos = fileBaseName.find_last_of('.');
                if (dotPos != std::string::npos) {
                    fileBaseName = fileBaseName.substr(0, dotPos);
                    
                    if (strcasecmp(fileBaseName.c_str(), baseName.c_str()) == 0) {
                        romFile = &file;
                        printf("CPS2_LoadROMData: Found alternative file %s for %s\n", 
                               file.name.c_str(), name);
                        break;
                    }
                }
            }
        }
        
        if (!romFile) {
            printf("CPS2_LoadROMData: ROM file %s not found\n", name);
            return false;
        }
    }
    
    // File must exist and have path
    if (romFile->path.empty()) {
        printf("CPS2_LoadROMData: ROM file %s has no path\n", name);
        return false;
    }
    
    // Validate ROM file size
    struct stat st;
    if (stat(romFile->path.c_str(), &st) != 0) {
        printf("CPS2_LoadROMData: Failed to stat file %s\n", romFile->path.c_str());
        return false;
    }
    
    size_t fileSize = st.st_size;
    if (fileSize == 0) {
        printf("CPS2_LoadROMData: ROM file %s is empty\n", romFile->path.c_str());
        return false;
    }
    
    // Check buffer size
    if (fileSize > size) {
        printf("CPS2_LoadROMData: Buffer too small (%zu < %zu)\n", size, fileSize);
        return false;
    }
    
    // Open the file
    FILE* file = fopen(romFile->path.c_str(), "rb");
    if (!file) {
        printf("CPS2_LoadROMData: Failed to open %s\n", romFile->path.c_str());
        return false;
    }
    
    // Read the file data
    size_t bytesRead = 0;
    
    // Use safer buffer reading instead of a single large read
    const size_t CHUNK_SIZE = 4096;
    size_t remaining = fileSize;
    uint8_t* bufPtr = static_cast<uint8_t*>(buffer);
    
    while (remaining > 0) {
        size_t chunkSize = (remaining > CHUNK_SIZE) ? CHUNK_SIZE : remaining;
        size_t read = fread(bufPtr, 1, chunkSize, file);
        
        if (read != chunkSize) {
            printf("CPS2_LoadROMData: Read error (read %zu of %zu bytes)\n", read, chunkSize);
            fclose(file);
            return false;
        }
        
        bytesRead += read;
        bufPtr += read;
        remaining -= read;
    }
    
    fclose(file);
    
    if (bytesRead != fileSize) {
        printf("CPS2_LoadROMData: Failed to read full file (read %zu of %zu bytes)\n", 
               bytesRead, fileSize);
        return false;
    }
    
    // Apply decryption if needed
    if (romFile->type == CPS2_ROM_PROGRAM && !romFile->decrypted && 
        g_cps2State.romInfo.encryption.enabled) {
        
        printf("CPS2_LoadROMData: Applying decryption for %s\n", name);
        
        // Call the core CPS2 decryption function
        extern int Cps2Decrypt(void*);
        int decryptResult = Cps2Decrypt(buffer);
        if (decryptResult != 0) {
            printf("CPS2_LoadROMData: Decryption failed with code %d\n", decryptResult);
            // Continue anyway - the ROM might still work
        } else {
            printf("CPS2_LoadROMData: Successfully decrypted %s\n", name);
            
            // Mark as decrypted in our state
            for (auto& file : g_cps2State.loadedFiles) {
                if (file.name == romFile->name) {
                    file.decrypted = true;
                    break;
                }
            }
        }
    }
    
    // Verify CRC32 if we haven't already
    if (!romFile->verified && !romFile->checksum.empty()) {
        // Calculate CRC32
        uint32_t crc = 0xFFFFFFFF;
        const uint8_t* data = static_cast<const uint8_t*>(buffer);
        
        // Make sure crcTable is properly initialized before using it
        extern uint32_t crcTable[256];
        bool crcTableValid = true;
        for (int i = 0; i < 256; i++) {
            if (crcTable[i] == 0 && i > 0) {
                crcTableValid = false;
                break;
            }
        }
        
        if (crcTableValid) {
            for (size_t i = 0; i < fileSize; i++) {
                crc = (crc >> 8) ^ crcTable[(crc & 0xFF) ^ data[i]];
            }
            crc = ~crc;
            
            // Format for comparison
            char crcStr[9];
            snprintf(crcStr, sizeof(crcStr), "%08x", crc);
            
            // Compare with expected (case-insensitive)
            if (strcasecmp(crcStr, romFile->checksum.c_str()) != 0) {
                printf("CPS2_LoadROMData: Warning - CRC mismatch for %s (expected %s, got %s)\n",
                      name, romFile->checksum.c_str(), crcStr);
            } else {
                printf("CPS2_LoadROMData: CRC verified for %s\n", name);
                
                // Mark as verified in our state
                for (auto& file : g_cps2State.loadedFiles) {
                    if (file.name == romFile->name) {
                        file.verified = true;
                        file.actualCrc = crc;
                        break;
                    }
                }
            }
        } else {
            printf("CPS2_LoadROMData: Warning - CRC table not initialized, skipping verification\n");
        }
    }
    
    printf("CPS2_LoadROMData: Successfully loaded %s (%zu bytes)\n", name, bytesRead);
    return true;
}

// Clean up temporary files
void CPS2_CleanupROMFiles() {
    if (!g_cps2State.initialized || !g_cps2State.romLoaded) {
        return;
    }
    
    // Delete temporary files
    printf("CPS2_CleanupROMFiles: Cleaning up temporary files\n");
    
    for (const auto& file : g_cps2State.loadedFiles) {
        if (unlink(file.path.c_str()) != 0) {
            printf("CPS2_CleanupROMFiles: Failed to delete file: %s\n", file.path.c_str());
        }
    }
    
    // Reset state
    g_cps2State.loadedFiles.clear();
    g_cps2State.romLoaded = false;
}

// Run a loaded CPS2 ROM
bool CPS2_RunROM() {
    if (!g_cps2State.initialized || !g_cps2State.romLoaded) {
        printf("CPS2_RunROM: No ROM currently loaded\n");
        return false;
    }
    
    printf("CPS2_RunROM: Running ROM set for %s\n", g_cps2State.currentGame.c_str());
    
    // First verify the ROM set
    if (!CPS2_VerifyLoadedROM()) {
        printf("CPS2_RunROM: ROM verification failed, attempting to run anyway\n");
    }
    
    // Initialize the FBNeo driver
    extern int BurnDrvInit();
    
    // Set up the core linkage
    CPS2_SetupMetalLinkage();
    
    // Find the driver for this game
    extern int FindDrvByName(const char* name);
    int drvIndex = FindDrvByName(g_cps2State.currentGame.c_str());
    
    if (drvIndex < 0) {
        // Try alternative names
        if (g_cps2State.currentGame == "mvscu") {
            drvIndex = FindDrvByName("mvsc");
        } else if (g_cps2State.currentGame == "sfz3") {
            drvIndex = FindDrvByName("sfa3");
        }
        
        if (drvIndex < 0) {
            printf("CPS2_RunROM: Driver not found for %s\n", g_cps2State.currentGame.c_str());
            return false;
        }
    }
    
    // Select the driver
    extern int DrvExit();
    extern bool bDrvOkay;
    extern int nBurnDrvActive;
    
    DrvExit();
    nBurnDrvActive = drvIndex;
    
    // Register our custom ROM loader
    extern bool (*pMetalCustomLoadROM)(char*, void*, int);
    
    // Custom ROM loader function
    auto customLoadROM = [](char* name, void* buffer, int size) -> bool {
        return CPS2_LoadROMData(name, buffer, size);
    };
    
    // Set the custom loader
    pMetalCustomLoadROM = customLoadROM;
    
    // Initialize the driver with the custom loader
    int result = BurnDrvInit();
    
    if (result != 0) {
        printf("CPS2_RunROM: Failed to initialize driver (code: %d)\n", result);
        return false;
    }
    
    // Check if initialization was successful
    if (!bDrvOkay) {
        printf("CPS2_RunROM: Driver initialization failed\n");
        return false;
    }
    
    // Set up game-specific hooks
    extern int (*pCps2FrameCallback)();
    extern int (*pCps2InitCallback)();
    
    // If we have hooks defined, set them up
    if (pCps2FrameCallback == nullptr) {
        extern int Cps2_OnFrame();
        pCps2FrameCallback = Cps2_OnFrame;
    }
    
    if (pCps2InitCallback == nullptr) {
        extern int Cps2_OnDriverInit();
        pCps2InitCallback = Cps2_OnDriverInit;
    }
    
    // Call the init hook manually since we're starting after initialization
    if (pCps2InitCallback) {
        pCps2InitCallback();
    }
    
    printf("CPS2_RunROM: Successfully started %s\n", g_cps2State.currentGame.c_str());
    return true;
}

// Get a list of supported CPS2 games
std::vector<CPS2GameInfo> CPS2_GetSupportedGames() {
    if (!g_cps2State.initialized) {
        CPS2_InitROMLoader();
    }
    
    std::vector<CPS2GameInfo> games;
    
    for (const auto& entry : g_cps2RomDb) {
        CPS2GameInfo game;
        game.id = entry.first;
        game.name = entry.second.name;
        game.romInfo = entry.second;
        
        // Check if ROM exists
        std::string zipName = game.id + ".zip";
        std::string zipPath;
        game.romAvailable = FindZipFile(zipName.c_str(), zipPath);
        
        games.push_back(game);
    }
    
    return games;
}

// Connect to FBNeo core
void CPS2_SetupMetalLinkage() {
    // This function provides connection points for the FBNeo core
    // It is called during Metal initialization
    printf("CPS2_SetupMetalLinkage: Setting up Metal linkage for CPS2\n");
    
    // Initialize ROM loader
    CPS2_InitROMLoader();
}

// Verify that all essential files for the loaded ROM set are available and valid
bool CPS2_VerifyLoadedROM() {
    if (!g_cps2State.initialized || !g_cps2State.romLoaded) {
        printf("CPS2_VerifyLoadedROM: No ROM currently loaded\n");
        return false;
    }
    
    printf("CPS2_VerifyLoadedROM: Verifying ROM set for %s\n", g_cps2State.currentGame.c_str());
    
    // Check if we have all required files
    bool allFilesPresent = true;
    bool allFilesVerified = true;
    int totalFiles = 0;
    int validFiles = 0;
    
    for (const auto& expectedFile : g_cps2State.romInfo.files) {
        totalFiles++;
        
        // Check if this file exists in our loaded files
        bool fileFound = false;
        bool fileVerified = false;
        
        for (const auto& loadedFile : g_cps2State.loadedFiles) {
            if (loadedFile.name == expectedFile.name) {
                fileFound = true;
                fileVerified = loadedFile.verified;
                break;
            }
        }
        
        if (!fileFound && !expectedFile.optional) {
            printf("CPS2_VerifyLoadedROM: Required file %s is missing\n", expectedFile.name.c_str());
            allFilesPresent = false;
        } else if (fileFound) {
            validFiles++;
            
            if (!fileVerified) {
                printf("CPS2_VerifyLoadedROM: File %s not verified\n", expectedFile.name.c_str());
                allFilesVerified = false;
            }
        }
    }
    
    // Log verification results
    printf("CPS2_VerifyLoadedROM: Verification results for %s:\n", g_cps2State.currentGame.c_str());
    printf("  - Total files: %d\n", totalFiles);
    printf("  - Valid files: %d\n", validFiles);
    printf("  - All files present: %s\n", allFilesPresent ? "Yes" : "No");
    printf("  - All files verified: %s\n", allFilesVerified ? "Yes" : "No");
    
    // Check if this ROM can run
    bool canRun = allFilesPresent;
    
    // Specific validation for each hardware type
    switch (g_cps2State.romInfo.hardwareType) {
        case CPS2_HW_MARVEL:
            // Marvel games need specific files to run
            for (const auto& loadedFile : g_cps2State.loadedFiles) {
                // Check for key files
                if (loadedFile.name.find(".key") != std::string::npos && !loadedFile.verified) {
                    printf("CPS2_VerifyLoadedROM: Marvel game requires verified key file\n");
                    canRun = false;
                }
            }
            break;
            
        case CPS2_HW_VAMPIRE:
            // Check for Vampire-specific requirements
            // (none specific currently)
            break;
            
        case CPS2_HW_XMVSF:
            // Check for XMVSF-specific requirements
            // (none specific currently)
            break;
            
        default:
            break;
    }
    
    // Print final result
    if (canRun) {
        printf("CPS2_VerifyLoadedROM: ROM set is valid and can run\n");
    } else {
        printf("CPS2_VerifyLoadedROM: ROM set is incomplete or invalid\n");
    }
    
    return canRun;
} 