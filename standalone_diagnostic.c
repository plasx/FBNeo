#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

// Simple self-contained ZIP file analyzer for FBNeo ROMs
// Doesn't depend on the main FBNeo build

// ZIP file header signatures
#define ZIP_LOCAL_HEADER_SIG 0x04034b50
#define ZIP_CENTRAL_DIR_SIG  0x02014b50
#define ZIP_END_OF_DIR_SIG   0x06054b50

// Type definitions for compatibility with FBNeo
typedef unsigned char  UINT8;
typedef unsigned short UINT16;
typedef unsigned int   UINT32;
typedef int            INT32;

// ZIP file header structures
#pragma pack(push, 1)
struct LocalFileHeader {
    UINT32 signature;         // 0x04034b50
    UINT16 versionNeeded;
    UINT16 flags;
    UINT16 compressionMethod;
    UINT16 lastModTime;
    UINT16 lastModDate;
    UINT32 crc32;
    UINT32 compressedSize;
    UINT32 uncompressedSize;
    UINT16 filenameLength;
    UINT16 extraFieldLength;
    // filename and extra field follow
};

struct CentralDirHeader {
    UINT32 signature;         // 0x02014b50
    UINT16 versionMade;
    UINT16 versionNeeded;
    UINT16 flags;
    UINT16 compressionMethod;
    UINT16 lastModTime;
    UINT16 lastModDate;
    UINT32 crc32;
    UINT32 compressedSize;
    UINT32 uncompressedSize;
    UINT16 filenameLength;
    UINT16 extraFieldLength;
    UINT16 commentLength;
    UINT16 diskStart;
    UINT16 internalAttrs;
    UINT32 externalAttrs;
    UINT32 localHeaderOffset;
    // filename, extra field, and comment follow
};

struct EndOfCentralDir {
    UINT32 signature;         // 0x06054b50
    UINT16 diskNumber;
    UINT16 centralDirDisk;
    UINT16 numEntriesOnDisk;
    UINT16 numEntriesTotal;
    UINT32 centralDirSize;
    UINT32 centralDirOffset;
    UINT16 commentLength;
    // comment follows
};
#pragma pack(pop)

// ROM file info
typedef struct {
    char filename[256];
    UINT32 compressedSize;
    UINT32 uncompressedSize;
    UINT32 crc32;
    UINT16 compressionMethod;
    UINT32 offset;
} ROMFile;

// Debugging levels
#define DEBUG_ERROR   0
#define DEBUG_WARNING 1
#define DEBUG_INFO    2
#define DEBUG_VERBOSE 3

// Global variables
static int g_debugLevel = DEBUG_INFO;
static FILE* g_logFile = NULL;

// Function prototypes
void initLog(const char* logName);
void closeLog();
void logMessage(int level, const char* format, ...);
void hexDump(const void* data, int size, const char* label);
int scanZipFile(const char* path, ROMFile** files, int* numFiles);
int extractFile(const char* zipPath, const ROMFile* file, UINT8** buffer, UINT32* size);
int analyzeROM(const char* path);

// Initialize log file
void initLog(const char* logName) {
    g_logFile = fopen(logName, "w");
    if (g_logFile) {
        fprintf(g_logFile, "=== ROM Diagnostic Tool Log ===\n");
        fprintf(g_logFile, "Time: %s\n", ctime(&(time_t){time(NULL)}));
        fprintf(g_logFile, "============================\n\n");
    }
}

// Close log file
void closeLog() {
    if (g_logFile) {
        fprintf(g_logFile, "\n=== End of Log ===\n");
        fclose(g_logFile);
        g_logFile = NULL;
    }
}

// Log a message
void logMessage(int level, const char* format, ...) {
    if (level > g_debugLevel) return;
    
    const char* levelNames[] = {
        "[ERROR] ",
        "[WARNING] ",
        "[INFO] ",
        "[VERBOSE] "
    };
    
    va_list args;
    va_start(args, format);
    
    // Print to console
    fprintf(stdout, "%s", levelNames[level]);
    vfprintf(stdout, format, args);
    fprintf(stdout, "\n");
    
    // Print to log file if open
    if (g_logFile) {
        va_end(args);
        va_start(args, format);
        fprintf(g_logFile, "%s", levelNames[level]);
        vfprintf(g_logFile, format, args);
        fprintf(g_logFile, "\n");
        fflush(g_logFile);
    }
    
    va_end(args);
}

// Dump memory in hex format
void hexDump(const void* data, int size, const char* label) {
    if (g_debugLevel < DEBUG_VERBOSE) return;
    
    const unsigned char* bytes = (const unsigned char*)data;
    logMessage(DEBUG_VERBOSE, "Memory dump of %s (%d bytes):", label, size);
    
    // Maximum bytes to show
    int maxBytes = (size > 256) ? 256 : size;
    
    char line[128];
    char* ptr;
    int len;
    
    for (int i = 0; i < maxBytes; i += 16) {
        ptr = line;
        len = snprintf(ptr, sizeof(line) - (ptr - line), "%04X: ", i);
        ptr += len;
        
        // Hex values
        for (int j = 0; j < 16; j++) {
            if (i + j < maxBytes) {
                len = snprintf(ptr, sizeof(line) - (ptr - line), "%02X ", bytes[i + j]);
                ptr += len;
            } else {
                len = snprintf(ptr, sizeof(line) - (ptr - line), "   ");
                ptr += len;
            }
            if (j == 7) {
                len = snprintf(ptr, sizeof(line) - (ptr - line), " ");
                ptr += len;
            }
        }
        
        // ASCII representation
        len = snprintf(ptr, sizeof(line) - (ptr - line), " |");
        ptr += len;
        
        for (int j = 0; j < 16; j++) {
            if (i + j < maxBytes) {
                unsigned char c = bytes[i + j];
                if (c >= 32 && c <= 126) {
                    len = snprintf(ptr, sizeof(line) - (ptr - line), "%c", c);
                } else {
                    len = snprintf(ptr, sizeof(line) - (ptr - line), ".");
                }
                ptr += len;
            } else {
                len = snprintf(ptr, sizeof(line) - (ptr - line), " ");
                ptr += len;
            }
        }
        
        snprintf(ptr, sizeof(line) - (ptr - line), "|");
        logMessage(DEBUG_VERBOSE, "%s", line);
    }
    
    if (size > maxBytes) {
        logMessage(DEBUG_VERBOSE, "... (truncated, %d more bytes)", size - maxBytes);
    }
}

// Scan a ZIP file for ROM contents
int scanZipFile(const char* path, ROMFile** files, int* numFiles) {
    FILE* file = fopen(path, "rb");
    if (!file) {
        logMessage(DEBUG_ERROR, "Failed to open ZIP file: %s", path);
        return 1;
    }
    
    logMessage(DEBUG_INFO, "Scanning ZIP file: %s", path);
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    logMessage(DEBUG_INFO, "File size: %ld bytes", fileSize);
    
    // Find end of central directory record
    const int maxCommentLength = 65535;
    long searchStart = fileSize < maxCommentLength ? 0 : fileSize - maxCommentLength;
    
    // Search for the end of central dir signature
    int foundEndOfDir = 0;
    EndOfCentralDir endOfDir;
    
    for (long pos = fileSize - sizeof(EndOfCentralDir); pos >= searchStart; pos--) {
        fseek(file, pos, SEEK_SET);
        if (fread(&endOfDir, 1, sizeof(EndOfCentralDir), file) != sizeof(EndOfCentralDir)) {
            continue;
        }
        
        if (endOfDir.signature == ZIP_END_OF_DIR_SIG) {
            foundEndOfDir = 1;
            logMessage(DEBUG_INFO, "Found end of central directory at offset %ld", pos);
            break;
        }
    }
    
    if (!foundEndOfDir) {
        logMessage(DEBUG_ERROR, "Failed to find ZIP central directory");
        fclose(file);
        return 1;
    }
    
    // Allocate memory for ROM file entries
    int maxFiles = endOfDir.numEntriesTotal;
    *files = (ROMFile*)malloc(maxFiles * sizeof(ROMFile));
    if (!*files) {
        logMessage(DEBUG_ERROR, "Failed to allocate memory for ROM files");
        fclose(file);
        return 1;
    }
    
    // Read central directory
    fseek(file, endOfDir.centralDirOffset, SEEK_SET);
    *numFiles = 0;
    
    logMessage(DEBUG_INFO, "Reading %d entries from central directory", maxFiles);
    
    for (int i = 0; i < maxFiles; i++) {
        CentralDirHeader header;
        if (fread(&header, 1, sizeof(CentralDirHeader), file) != sizeof(CentralDirHeader)) {
            logMessage(DEBUG_ERROR, "Error reading central directory header %d", i);
            break;
        }
        
        if (header.signature != ZIP_CENTRAL_DIR_SIG) {
            logMessage(DEBUG_ERROR, "Invalid central directory signature at entry %d", i);
            break;
        }
        
        // Read filename
        if (header.filenameLength >= sizeof((*files)[i].filename)) {
            logMessage(DEBUG_WARNING, "Filename too long for entry %d", i);
            fseek(file, header.filenameLength + header.extraFieldLength + header.commentLength, SEEK_CUR);
            continue;
        }
        
        if (fread((*files)[i].filename, 1, header.filenameLength, file) != header.filenameLength) {
            logMessage(DEBUG_ERROR, "Error reading filename for entry %d", i);
            break;
        }
        (*files)[i].filename[header.filenameLength] = '\0';
        
        // Skip extra field and comment
        fseek(file, header.extraFieldLength + header.commentLength, SEEK_CUR);
        
        // Store file information
        (*files)[i].offset = header.localHeaderOffset;
        (*files)[i].compressedSize = header.compressedSize;
        (*files)[i].uncompressedSize = header.uncompressedSize;
        (*files)[i].compressionMethod = header.compressionMethod;
        (*files)[i].crc32 = header.crc32;
        
        logMessage(DEBUG_VERBOSE, "Entry %d: %s (size: %u, compressed: %u, method: %d, CRC: 0x%08X)", 
                 i, (*files)[i].filename, header.uncompressedSize, header.compressedSize,
                 header.compressionMethod, header.crc32);
        
        (*numFiles)++;
    }
    
    fclose(file);
    logMessage(DEBUG_INFO, "Found %d files in ZIP archive", *numFiles);
    return 0;
}

// Extract a file from the ZIP
int extractFile(const char* zipPath, const ROMFile* file, UINT8** buffer, UINT32* size) {
    if (!zipPath || !file || !buffer || !size) {
        logMessage(DEBUG_ERROR, "Invalid parameters to extractFile");
        return 1;
    }
    
    logMessage(DEBUG_INFO, "Extracting file: %s", file->filename);
    
    FILE* zipFile = fopen(zipPath, "rb");
    if (!zipFile) {
        logMessage(DEBUG_ERROR, "Failed to open ZIP file for extraction");
        return 1;
    }
    
    // Go to local header
    fseek(zipFile, file->offset, SEEK_SET);
    
    // Read local header
    LocalFileHeader header;
    if (fread(&header, 1, sizeof(LocalFileHeader), zipFile) != sizeof(LocalFileHeader)) {
        logMessage(DEBUG_ERROR, "Failed to read local file header");
        fclose(zipFile);
        return 1;
    }
    
    if (header.signature != ZIP_LOCAL_HEADER_SIG) {
        logMessage(DEBUG_ERROR, "Invalid local file header signature");
        fclose(zipFile);
        return 1;
    }
    
    // Skip filename and extra field
    fseek(zipFile, header.filenameLength + header.extraFieldLength, SEEK_CUR);
    
    // Allocate buffer for uncompressed data
    *buffer = (UINT8*)malloc(file->uncompressedSize);
    if (!*buffer) {
        logMessage(DEBUG_ERROR, "Failed to allocate memory for uncompressed data");
        fclose(zipFile);
        return 1;
    }
    
    *size = file->uncompressedSize;
    
    // Extract based on compression method
    if (file->compressionMethod == 0) {
        // Stored (no compression)
        if (fread(*buffer, 1, file->uncompressedSize, zipFile) != file->uncompressedSize) {
            logMessage(DEBUG_ERROR, "Failed to read uncompressed data");
            free(*buffer);
            *buffer = NULL;
            fclose(zipFile);
            return 1;
        }
    } else if (file->compressionMethod == 8) {
        // DEFLATE compression
        UINT8* compressedData = (UINT8*)malloc(file->compressedSize);
        if (!compressedData) {
            logMessage(DEBUG_ERROR, "Failed to allocate memory for compressed data");
            free(*buffer);
            *buffer = NULL;
            fclose(zipFile);
            return 1;
        }
        
        // Read compressed data
        if (fread(compressedData, 1, file->compressedSize, zipFile) != file->compressedSize) {
            logMessage(DEBUG_ERROR, "Failed to read compressed data");
            free(compressedData);
            free(*buffer);
            *buffer = NULL;
            fclose(zipFile);
            return 1;
        }
        
        // Set up zlib stream
        z_stream stream;
        memset(&stream, 0, sizeof(stream));
        
        stream.next_in = compressedData;
        stream.avail_in = file->compressedSize;
        stream.next_out = *buffer;
        stream.avail_out = file->uncompressedSize;
        
        // Initialize for raw DEFLATE
        if (inflateInit2(&stream, -MAX_WBITS) != Z_OK) {
            logMessage(DEBUG_ERROR, "Failed to initialize zlib");
            free(compressedData);
            free(*buffer);
            *buffer = NULL;
            fclose(zipFile);
            return 1;
        }
        
        // Decompress
        int status = inflate(&stream, Z_FINISH);
        
        if (status != Z_STREAM_END) {
            logMessage(DEBUG_ERROR, "Failed to decompress data: %d", status);
            inflateEnd(&stream);
            free(compressedData);
            free(*buffer);
            *buffer = NULL;
            fclose(zipFile);
            return 1;
        }
        
        // Clean up
        inflateEnd(&stream);
        free(compressedData);
    } else {
        logMessage(DEBUG_ERROR, "Unsupported compression method: %d", file->compressionMethod);
        free(*buffer);
        *buffer = NULL;
        fclose(zipFile);
        return 1;
    }
    
    fclose(zipFile);
    
    // Calculate checksum of extracted data
    UINT32 checksum = 0;
    int nonZeroBytes = 0;
    
    for (UINT32 i = 0; i < file->uncompressedSize; i++) {
        checksum += (*buffer)[i];
        if ((*buffer)[i] != 0) nonZeroBytes++;
    }
    
    float nonZeroPercent = (float)nonZeroBytes / (float)file->uncompressedSize * 100.0f;
    
    logMessage(DEBUG_INFO, "Extraction successful:");
    logMessage(DEBUG_INFO, "  Size: %u bytes", file->uncompressedSize);
    logMessage(DEBUG_INFO, "  Checksum: 0x%08X", checksum);
    logMessage(DEBUG_INFO, "  Non-zero bytes: %d (%.2f%%)", nonZeroBytes, nonZeroPercent);
    
    // Dump first bytes
    hexDump(*buffer, file->uncompressedSize > 64 ? 64 : file->uncompressedSize, file->filename);
    
    return 0;
}

// Analyze a ROM file
int analyzeROM(const char* path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        logMessage(DEBUG_ERROR, "File does not exist: %s", path);
        return 1;
    }
    
    logMessage(DEBUG_INFO, "Analyzing ROM file: %s", path);
    logMessage(DEBUG_INFO, "File size: %lld bytes", (long long)st.st_size);
    logMessage(DEBUG_INFO, "Last modified: %s", ctime(&st.st_mtime));
    
    // Check if it's a ZIP file
    if (strstr(path, ".zip") || strstr(path, ".ZIP")) {
        ROMFile* files = NULL;
        int numFiles = 0;
        
        // Scan the ZIP file
        if (scanZipFile(path, &files, &numFiles) != 0) {
            logMessage(DEBUG_ERROR, "Failed to scan ZIP file");
            return 1;
        }
        
        // Print ZIP contents
        logMessage(DEBUG_INFO, "ZIP contains %d files:", numFiles);
        
        for (int i = 0; i < numFiles; i++) {
            logMessage(DEBUG_INFO, "[%d] %s (Size: %u bytes, CRC32: 0x%08X)", 
                      i, files[i].filename, files[i].uncompressedSize, files[i].crc32);
        }
        
        // Extract and analyze common ROM types
        for (int i = 0; i < numFiles; i++) {
            // Look for typical ROM files with these extensions
            const char* extensions[] = { ".bin", ".rom", ".cpr", ".u", ".ic", ".spr", ".88", ".8", ".68k", NULL };
            int shouldExtract = 0;
            
            // Check if the current file has one of the target extensions
            for (int j = 0; extensions[j] != NULL; j++) {
                if (strstr(files[i].filename, extensions[j])) {
                    shouldExtract = 1;
                    break;
                }
            }
            
            // Also extract any file over 16KB (likely ROM data)
            if (files[i].uncompressedSize > 16384) {
                shouldExtract = 1;
            }
            
            if (shouldExtract && (files[i].compressionMethod == 0 || files[i].compressionMethod == 8)) {
                logMessage(DEBUG_INFO, "Analyzing ROM file: %s", files[i].filename);
                
                UINT8* buffer = NULL;
                UINT32 size = 0;
                
                if (extractFile(path, &files[i], &buffer, &size) == 0) {
                    // Check for typical ROM patterns
                    int hasROMSignature = 0;
                    
                    // Check for common header bytes for various systems
                    if (size > 16) {
                        // Check for 68K code (common in arcade ROMs)
                        if (buffer[0] == 0x46 && buffer[1] == 0xFC) {
                            logMessage(DEBUG_INFO, "  Possible 68K program ROM (starts with 46 FC)");
                            hasROMSignature = 1;
                        }
                        
                        // Check for Z80 code (common in arcade ROMs)
                        if (buffer[0] == 0xC3 || buffer[0] == 0x18 || buffer[0] == 0x00) {
                            logMessage(DEBUG_INFO, "  Possible Z80 program ROM");
                            hasROMSignature = 1;
                        }
                    }
                    
                    if (!hasROMSignature) {
                        logMessage(DEBUG_INFO, "  No specific ROM signature detected, may be graphics or generic data");
                    }
                    
                    // Free buffer
                    free(buffer);
                }
            }
        }
        
        // Free files
        free(files);
    } else {
        logMessage(DEBUG_ERROR, "File is not a ZIP archive");
        return 1;
    }
    
    return 0;
}

// Main function
int main(int argc, char** argv) {
    // Parse command line
    int verbose = 0;
    const char* romPath = NULL;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else if (romPath == NULL) {
            romPath = argv[i];
        }
    }
    
    if (romPath == NULL) {
        printf("Usage: %s [options] <rom_file.zip>\n", argv[0]);
        printf("Options:\n");
        printf("  -v, --verbose   Enable verbose output\n");
        return 1;
    }
    
    // Set debug level
    g_debugLevel = verbose ? DEBUG_VERBOSE : DEBUG_INFO;
    
    // Create output directory
    mkdir("debug_output", 0755);
    
    // Initialize log
    initLog("debug_output/rom_analysis.log");
    
    // Analyze ROM
    int result = analyzeROM(romPath);
    
    // Close log
    closeLog();
    
    return result;
} 