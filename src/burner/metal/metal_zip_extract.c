#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#include "metal_declarations.h"
#include "metal_zip_extract.h"
#include "rom_loading_debug.h"

// Basic ZIP file format structures
typedef struct {
    unsigned int signature;         // 0x04034b50
    unsigned short version;         // Version needed to extract
    unsigned short flags;           // General purpose bit flag
    unsigned short compression;     // Compression method
    unsigned short last_mod_time;   // Last mod file time
    unsigned short last_mod_date;   // Last mod file date
    unsigned int crc32;             // CRC-32
    unsigned int compressed_size;   // Compressed size
    unsigned int uncompressed_size; // Uncompressed size
    unsigned short filename_length; // File name length
    unsigned short extra_field_length; // Extra field length
} ZIP_LocalFileHeader;

// Simple ZIP central directory header
typedef struct {
    unsigned int signature;         // 0x02014b50
    unsigned short version_made;    // Version made by
    unsigned short version_needed;  // Version needed to extract
    unsigned short flags;           // General purpose bit flag
    unsigned short compression;     // Compression method
    unsigned short last_mod_time;   // Last mod file time
    unsigned short last_mod_date;   // Last mod file date
    unsigned int crc32;             // CRC-32
    unsigned int compressed_size;   // Compressed size
    unsigned int uncompressed_size; // Uncompressed size
    unsigned short filename_length; // File name length
    unsigned short extra_field_length; // Extra field length
    unsigned short comment_length;  // File comment length
    unsigned short disk_number;     // Disk number start
    unsigned short internal_attrs;  // Internal file attributes
    unsigned int external_attrs;    // External file attributes
    unsigned int local_header_offset; // Relative offset of local header
} ZIP_CentralDirHeader;

// End of central directory record
typedef struct {
    unsigned int signature;         // 0x06054b50
    unsigned short disk_number;     // Number of this disk
    unsigned short start_disk;      // Disk where central directory starts
    unsigned short disk_entries;    // Number of central directory records on this disk
    unsigned short total_entries;   // Total number of central directory records
    unsigned int central_dir_size;  // Size of central directory (bytes)
    unsigned int central_dir_offset; // Offset of start of central directory
    unsigned short comment_length;  // Comment length
} ZIP_EndOfCentralDir;

// Extract a file from a ZIP archive
int Metal_ExtractFileFromZip(const char* zipPath, const char* filename, UINT8* buffer, int bufferSize, int* bytesExtracted) {
    FILE* file = NULL;
    unsigned char* tempBuffer = NULL;
    int result = -1;
    
    if (!zipPath || !filename || !buffer || bufferSize <= 0) {
        ROMLoader_DebugLog(LOG_ERROR, "Invalid parameters for Metal_ExtractFileFromZip");
        return -1;
    }
    
    ROMLoader_DebugLog(LOG_INFO, "Extracting %s from %s", filename, zipPath);
    
    // Set initial value for bytesExtracted
    if (bytesExtracted) {
        *bytesExtracted = 0;
    }
    
    // Open ZIP file
    file = fopen(zipPath, "rb");
    if (!file) {
        ROMLoader_DebugLog(LOG_ERROR, "Failed to open ZIP file: %s", zipPath);
        return -1;
    }
    
    // First, find the central directory by seeking to the end and looking for the signature
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    
    // Allocate a small buffer to search for the end of central directory record
    // We'll look in the last 4KB of the file (or the whole file if smaller)
    const int searchSize = (fileSize < 4096) ? (int)fileSize : 4096;
    tempBuffer = (unsigned char*)malloc(searchSize);
    if (!tempBuffer) {
        ROMLoader_DebugLog(LOG_ERROR, "Memory allocation failed");
        fclose(file);
        return -1;
    }
    
    // Read the last part of the file
    fseek(file, fileSize - searchSize, SEEK_SET);
    if (fread(tempBuffer, 1, searchSize, file) != (size_t)searchSize) {
        ROMLoader_DebugLog(LOG_ERROR, "Failed to read end of ZIP file");
        free(tempBuffer);
        fclose(file);
        return -1;
    }
    
    // Look for the End of Central Directory signature (0x06054b50)
    int eocOffset = -1;
    for (int i = searchSize - 4; i >= 0; i--) {
        if (tempBuffer[i] == 0x50 && tempBuffer[i+1] == 0x4b && 
            tempBuffer[i+2] == 0x05 && tempBuffer[i+3] == 0x06) {
            eocOffset = i;
            break;
        }
    }
    
    if (eocOffset == -1) {
        ROMLoader_DebugLog(LOG_ERROR, "End of Central Directory not found");
        free(tempBuffer);
        fclose(file);
        return -1;
    }
    
    // Read the End of Central Directory record
    ZIP_EndOfCentralDir eoc;
    memcpy(&eoc, tempBuffer + eocOffset, sizeof(ZIP_EndOfCentralDir));
    free(tempBuffer);
    
    // Position of Central Directory in the file
    long centralDirPos = eoc.central_dir_offset;
    
    // Now seek to central directory
    fseek(file, centralDirPos, SEEK_SET);
    
    // Allocate a buffer for the central directory
    tempBuffer = (unsigned char*)malloc(eoc.central_dir_size);
    if (!tempBuffer) {
        ROMLoader_DebugLog(LOG_ERROR, "Memory allocation failed for central directory");
        fclose(file);
        return -1;
    }
    
    // Read the central directory
    if (fread(tempBuffer, 1, eoc.central_dir_size, file) != eoc.central_dir_size) {
        ROMLoader_DebugLog(LOG_ERROR, "Failed to read central directory");
        free(tempBuffer);
        fclose(file);
        return -1;
    }
    
    // Process the central directory entries to find our file
    unsigned char* p = tempBuffer;
    int fileFound = 0;
    ZIP_CentralDirHeader cdh;
    
    for (int i = 0; i < eoc.total_entries; i++) {
        memcpy(&cdh, p, sizeof(ZIP_CentralDirHeader));
        
        // Move past the header
        p += sizeof(ZIP_CentralDirHeader);
        
        // Get the filename
        char* entryFilename = (char*)malloc(cdh.filename_length + 1);
        if (!entryFilename) {
            ROMLoader_DebugLog(LOG_ERROR, "Memory allocation failed for filename");
            free(tempBuffer);
            fclose(file);
            return -1;
        }
        
        memcpy(entryFilename, p, cdh.filename_length);
        entryFilename[cdh.filename_length] = '\0';
        
        // Move past filename, extra field, and comment
        p += cdh.filename_length + cdh.extra_field_length + cdh.comment_length;
        
        // Check if this is the file we're looking for
        if (strcmp(entryFilename, filename) == 0) {
            fileFound = 1;
            free(entryFilename);
            break;
        }
        
        free(entryFilename);
    }
    
    if (!fileFound) {
        ROMLoader_DebugLog(LOG_ERROR, "File %s not found in ZIP", filename);
        free(tempBuffer);
        fclose(file);
        return -1;
    }
    
    // Now extract the file by seeking to the local file header
    fseek(file, cdh.local_header_offset, SEEK_SET);
    
    // Read the local file header
    ZIP_LocalFileHeader lfh;
    if (fread(&lfh, sizeof(ZIP_LocalFileHeader), 1, file) != 1) {
        ROMLoader_DebugLog(LOG_ERROR, "Failed to read local file header");
        free(tempBuffer);
        fclose(file);
        return -1;
    }
    
    // Skip past filename and extra field
    fseek(file, lfh.filename_length + lfh.extra_field_length, SEEK_CUR);
    
    // Now we're at the file data
    // Check if compressed size is larger than our buffer
    if (lfh.compressed_size > (unsigned int)bufferSize && lfh.compression != 0) {
        ROMLoader_DebugLog(LOG_ERROR, "Compressed data size (%u) is larger than buffer size (%d)", 
                         lfh.compressed_size, bufferSize);
        free(tempBuffer);
        fclose(file);
        return -1;
    }
    
    // Check if uncompressed size is larger than our buffer
    if (lfh.uncompressed_size > (unsigned int)bufferSize) {
        ROMLoader_DebugLog(LOG_ERROR, "Uncompressed data size (%u) is larger than buffer size (%d)", 
                         lfh.uncompressed_size, bufferSize);
        free(tempBuffer);
        fclose(file);
        return -1;
    }
    
    // Read and extract data based on compression method
    if (lfh.compression == 0) {
        // Store method (no compression)
        if (fread(buffer, 1, lfh.uncompressed_size, file) != lfh.uncompressed_size) {
            ROMLoader_DebugLog(LOG_ERROR, "Failed to read uncompressed data");
            free(tempBuffer);
            fclose(file);
            return -1;
        }
        
        if (bytesExtracted) {
            *bytesExtracted = lfh.uncompressed_size;
        }
        
        ROMLoader_DebugLog(LOG_INFO, "Successfully extracted %s (%u bytes)", 
                         filename, lfh.uncompressed_size);
        result = 0;
    } else if (lfh.compression == 8) {
        // Deflate method
        free(tempBuffer);
        tempBuffer = (unsigned char*)malloc(lfh.compressed_size);
        if (!tempBuffer) {
            ROMLoader_DebugLog(LOG_ERROR, "Memory allocation failed for compressed data");
            fclose(file);
            return -1;
        }
        
        // Read compressed data
        if (fread(tempBuffer, 1, lfh.compressed_size, file) != lfh.compressed_size) {
            ROMLoader_DebugLog(LOG_ERROR, "Failed to read compressed data");
            free(tempBuffer);
            fclose(file);
            return -1;
        }
        
        // Decompress using zlib
        z_stream stream;
        memset(&stream, 0, sizeof(stream));
        
        stream.next_in = tempBuffer;
        stream.avail_in = lfh.compressed_size;
        stream.next_out = buffer;
        stream.avail_out = bufferSize;
        
        // Initialize zlib for inflation
        if (inflateInit2(&stream, -MAX_WBITS) != Z_OK) {
            ROMLoader_DebugLog(LOG_ERROR, "Failed to initialize zlib");
            free(tempBuffer);
            fclose(file);
            return -1;
        }
        
        // Decompress
        int zresult = inflate(&stream, Z_FINISH);
        if (zresult != Z_STREAM_END) {
            ROMLoader_DebugLog(LOG_ERROR, "Failed to decompress data: %d", zresult);
            inflateEnd(&stream);
            free(tempBuffer);
            fclose(file);
            return -1;
        }
        
        // Get the decompressed size
        unsigned int decompressedSize = stream.total_out;
        
        // Clean up zlib
        inflateEnd(&stream);
        
        if (bytesExtracted) {
            *bytesExtracted = decompressedSize;
        }
        
        ROMLoader_DebugLog(LOG_INFO, "Successfully extracted and decompressed %s (%u bytes)", 
                         filename, decompressedSize);
        result = 0;
    } else {
        ROMLoader_DebugLog(LOG_ERROR, "Unsupported compression method: %d", lfh.compression);
        result = -1;
    }
    
    // Verify the extracted data
    unsigned int calculatedCrc = 0;
    if (result == 0) {
        // Using a simple CRC calculation instead of zlib's
        unsigned int uncompressedSize = bytesExtracted ? *bytesExtracted : lfh.uncompressed_size;
        for (unsigned int i = 0; i < uncompressedSize; i++) {
            calculatedCrc = (calculatedCrc >> 1) ^ (-(calculatedCrc & 1) & 0xEDB88320) ^ buffer[i];
        }
        
        if (calculatedCrc != lfh.crc32) {
            ROMLoader_DebugLog(LOG_WARNING, "CRC mismatch! Expected 0x%08X, got 0x%08X", 
                             lfh.crc32, calculatedCrc);
        } else {
            ROMLoader_DebugLog(LOG_INFO, "CRC verification passed: 0x%08X", calculatedCrc);
        }
        
        // Dump first part of data for debugging
        ROMLoader_DumpMemory(buffer, uncompressedSize > 64 ? 64 : uncompressedSize, filename);
    }
    
    // Clean up and return
    free(tempBuffer);
    fclose(file);
    return result;
}

// Get information about a file in a ZIP without extracting it
int Metal_GetZipFileInfo(const char* zipPath, const char* filename, UINT32* size, UINT32* crc) {
    FILE* file = NULL;
    unsigned char* tempBuffer = NULL;
    int result = -1;
    
    if (!zipPath || !filename) {
        return -1;
    }
    
    // Open ZIP file
    file = fopen(zipPath, "rb");
    if (!file) {
        return -1;
    }
    
    // First, find the central directory
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    
    // Allocate a small buffer to search for the end of central directory record
    const int searchSize = (fileSize < 4096) ? (int)fileSize : 4096;
    tempBuffer = (unsigned char*)malloc(searchSize);
    if (!tempBuffer) {
        fclose(file);
        return -1;
    }
    
    // Read the last part of the file
    fseek(file, fileSize - searchSize, SEEK_SET);
    if (fread(tempBuffer, 1, searchSize, file) != (size_t)searchSize) {
        free(tempBuffer);
        fclose(file);
        return -1;
    }
    
    // Look for the End of Central Directory signature
    int eocOffset = -1;
    for (int i = searchSize - 4; i >= 0; i--) {
        if (tempBuffer[i] == 0x50 && tempBuffer[i+1] == 0x4b && 
            tempBuffer[i+2] == 0x05 && tempBuffer[i+3] == 0x06) {
            eocOffset = i;
            break;
        }
    }
    
    if (eocOffset == -1) {
        free(tempBuffer);
        fclose(file);
        return -1;
    }
    
    // Read the End of Central Directory record
    ZIP_EndOfCentralDir eoc;
    memcpy(&eoc, tempBuffer + eocOffset, sizeof(ZIP_EndOfCentralDir));
    free(tempBuffer);
    
    // Position of Central Directory in the file
    long centralDirPos = eoc.central_dir_offset;
    
    // Now seek to central directory
    fseek(file, centralDirPos, SEEK_SET);
    
    // Allocate a buffer for the central directory
    tempBuffer = (unsigned char*)malloc(eoc.central_dir_size);
    if (!tempBuffer) {
        fclose(file);
        return -1;
    }
    
    // Read the central directory
    if (fread(tempBuffer, 1, eoc.central_dir_size, file) != eoc.central_dir_size) {
        free(tempBuffer);
        fclose(file);
        return -1;
    }
    
    // Process the central directory entries to find our file
    unsigned char* p = tempBuffer;
    int fileFound = 0;
    ZIP_CentralDirHeader cdh;
    
    for (int i = 0; i < eoc.total_entries; i++) {
        memcpy(&cdh, p, sizeof(ZIP_CentralDirHeader));
        
        // Move past the header
        p += sizeof(ZIP_CentralDirHeader);
        
        // Get the filename
        char* entryFilename = (char*)malloc(cdh.filename_length + 1);
        if (!entryFilename) {
            free(tempBuffer);
            fclose(file);
            return -1;
        }
        
        memcpy(entryFilename, p, cdh.filename_length);
        entryFilename[cdh.filename_length] = '\0';
        
        // Move past filename, extra field, and comment
        p += cdh.filename_length + cdh.extra_field_length + cdh.comment_length;
        
        // Check if this is the file we're looking for
        if (strcmp(entryFilename, filename) == 0) {
            fileFound = 1;
            
            // Fill in size and CRC if requested
            if (size) {
                *size = cdh.uncompressed_size;
            }
            if (crc) {
                *crc = cdh.crc32;
            }
            
            free(entryFilename);
            result = 0;
            break;
        }
        
        free(entryFilename);
    }
    
    // Clean up and return
    free(tempBuffer);
    fclose(file);
    return result;
}

// List files in a ZIP archive
int Metal_ListZipContents(const char* zipPath, char** filenames, int maxFiles, int* numFiles) {
    FILE* file = NULL;
    unsigned char* tempBuffer = NULL;
    int result = -1;
    
    if (!zipPath || !filenames || maxFiles <= 0 || !numFiles) {
        return -1;
    }
    
    // Initialize numFiles
    *numFiles = 0;
    
    // Open ZIP file
    file = fopen(zipPath, "rb");
    if (!file) {
        return -1;
    }
    
    // First, find the central directory
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    
    // Allocate a small buffer to search for the end of central directory record
    const int searchSize = (fileSize < 4096) ? (int)fileSize : 4096;
    tempBuffer = (unsigned char*)malloc(searchSize);
    if (!tempBuffer) {
        fclose(file);
        return -1;
    }
    
    // Read the last part of the file
    fseek(file, fileSize - searchSize, SEEK_SET);
    if (fread(tempBuffer, 1, searchSize, file) != (size_t)searchSize) {
        free(tempBuffer);
        fclose(file);
        return -1;
    }
    
    // Look for the End of Central Directory signature
    int eocOffset = -1;
    for (int i = searchSize - 4; i >= 0; i--) {
        if (tempBuffer[i] == 0x50 && tempBuffer[i+1] == 0x4b && 
            tempBuffer[i+2] == 0x05 && tempBuffer[i+3] == 0x06) {
            eocOffset = i;
            break;
        }
    }
    
    if (eocOffset == -1) {
        free(tempBuffer);
        fclose(file);
        return -1;
    }
    
    // Read the End of Central Directory record
    ZIP_EndOfCentralDir eoc;
    memcpy(&eoc, tempBuffer + eocOffset, sizeof(ZIP_EndOfCentralDir));
    free(tempBuffer);
    
    // Position of Central Directory in the file
    long centralDirPos = eoc.central_dir_offset;
    
    // Now seek to central directory
    fseek(file, centralDirPos, SEEK_SET);
    
    // Allocate a buffer for the central directory
    tempBuffer = (unsigned char*)malloc(eoc.central_dir_size);
    if (!tempBuffer) {
        fclose(file);
        return -1;
    }
    
    // Read the central directory
    if (fread(tempBuffer, 1, eoc.central_dir_size, file) != eoc.central_dir_size) {
        free(tempBuffer);
        fclose(file);
        return -1;
    }
    
    // Process the central directory entries to get filenames
    unsigned char* p = tempBuffer;
    int count = 0;
    ZIP_CentralDirHeader cdh;
    
    for (int i = 0; i < eoc.total_entries && count < maxFiles; i++) {
        memcpy(&cdh, p, sizeof(ZIP_CentralDirHeader));
        
        // Move past the header
        p += sizeof(ZIP_CentralDirHeader);
        
        // Get the filename
        if (cdh.filename_length > 0) {
            memcpy(filenames[count], p, cdh.filename_length);
            filenames[count][cdh.filename_length] = '\0';
            count++;
        }
        
        // Move past filename, extra field, and comment
        p += cdh.filename_length + cdh.extra_field_length + cdh.comment_length;
    }
    
    *numFiles = count;
    result = 0;
    
    // Clean up and return
    free(tempBuffer);
    fclose(file);
    return result;
} 