#import <Foundation/Foundation.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#import <Compression/Compression.h>
#import <zlib.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Structure for ZIP file header
typedef struct {
    uint32_t signature;      // 0x04034b50
    uint16_t version;
    uint16_t flags;
    uint16_t compression;    // 0=none, 8=deflate
    uint16_t modTime;
    uint16_t modDate;
    uint32_t crc32;
    uint32_t compressedSize;
    uint32_t uncompressedSize;
    uint16_t filenameLength;
    uint16_t extraFieldLength;
    // Followed by filename, extra field, and file data
} ZipLocalFileHeader;

// Helper function to extract file from ZIP data
static void* ExtractFileFromZipData(NSData* zipData, const char* fileName, int* fileSize) {
    if (!zipData || !fileName || !fileSize) {
        return NULL;
    }
    
    NSLog(@"Extracting file: %s from ZIP data (%lu bytes)", fileName, (unsigned long)zipData.length);
    
    const uint8_t* bytes = (const uint8_t*)zipData.bytes;
    const uint8_t* end = bytes + zipData.length;
    uint8_t* fileData = NULL;
    *fileSize = 0;
    
    // Loop through the ZIP archive looking for the requested file
    while (bytes + sizeof(ZipLocalFileHeader) <= end) {
        const ZipLocalFileHeader* header = (const ZipLocalFileHeader*)bytes;
        
        // Check for local file header signature (0x04034b50)
        if (header->signature != 0x04034b50) {
            bytes++;
            continue;
        }
        
        // Extract filename
        uint16_t filenameLength = CFSwapInt16LittleToHost(header->filenameLength);
        if (bytes + sizeof(ZipLocalFileHeader) + filenameLength > end) {
            break;
        }
        
        // Get filename from header
        char* headerFileName = (char*)malloc(filenameLength + 1);
        if (!headerFileName) {
            break;
        }
        memcpy(headerFileName, bytes + sizeof(ZipLocalFileHeader), filenameLength);
        headerFileName[filenameLength] = '\0';
        
        // Check if this is the file we're looking for
        bool isMatch = (strcmp(headerFileName, fileName) == 0);
        
        // Free the filename memory
        free(headerFileName);
        
        // If we found the file, extract it
        if (isMatch) {
            uint16_t compression = CFSwapInt16LittleToHost(header->compression);
            uint32_t compressedSize = CFSwapInt32LittleToHost(header->compressedSize);
            uint32_t uncompressedSize = CFSwapInt32LittleToHost(header->uncompressedSize);
            uint16_t extraFieldLength = CFSwapInt16LittleToHost(header->extraFieldLength);
            
            NSLog(@"Found file: %s (compressed: %u, uncompressed: %u, method: %u)",
                  fileName, compressedSize, uncompressedSize, compression);
            
            // Calculate offset to file data
            const uint8_t* fileDataStart = bytes + sizeof(ZipLocalFileHeader) + filenameLength + extraFieldLength;
            
            // Check if we have enough data
            if (fileDataStart + compressedSize > end) {
                NSLog(@"Error: ZIP file data exceeds buffer bounds");
                break;
            }
            
            // Allocate memory for the extracted file
            fileData = (uint8_t*)malloc(uncompressedSize);
            if (!fileData) {
                NSLog(@"Error: Failed to allocate memory for extracted file");
                break;
            }
            
            // Extract the file based on compression method
            if (compression == 0) {
                // No compression, just copy the data
                memcpy(fileData, fileDataStart, uncompressedSize);
                *fileSize = uncompressedSize;
                NSLog(@"File extracted (uncompressed)");
            } else if (compression == 8) {
                // Deflate compression
                z_stream stream;
                memset(&stream, 0, sizeof(z_stream));
                
                stream.next_in = (Bytef*)fileDataStart;
                stream.avail_in = compressedSize;
                stream.next_out = fileData;
                stream.avail_out = uncompressedSize;
                
                // Initialize zlib for raw deflate (negative window bits)
                if (inflateInit2(&stream, -15) != Z_OK) {
                    NSLog(@"Error: Failed to initialize zlib");
                    free(fileData);
                    fileData = NULL;
                    break;
                }
                
                // Decompress the data
                int result = inflate(&stream, Z_FINISH);
                inflateEnd(&stream);
                
                if (result != Z_STREAM_END) {
                    NSLog(@"Error: Failed to decompress file data (result: %d)", result);
                    free(fileData);
                    fileData = NULL;
                    break;
                }
                
                *fileSize = uncompressedSize;
                NSLog(@"File extracted and decompressed");
            } else {
                NSLog(@"Error: Unsupported compression method: %u", compression);
                free(fileData);
                fileData = NULL;
                break;
            }
            
            // File found and extracted, exit the loop
            break;
        }
        
        // Move to the next file header
        uint16_t extraFieldLength = CFSwapInt16LittleToHost(header->extraFieldLength);
        uint32_t compressedSize = CFSwapInt32LittleToHost(header->compressedSize);
        bytes += sizeof(ZipLocalFileHeader) + filenameLength + extraFieldLength + compressedSize;
    }
    
    return fileData;
}

// C-callable function to load a file from a ZIP archive
extern "C" void* LoadFileFromZip(const char* zipPath, const char* fileName, int* fileSize) {
    if (!zipPath || !fileName || !fileSize) {
        return NULL;
    }
    
    *fileSize = 0;
    
    @autoreleasepool {
        NSString* zipPathStr = [NSString stringWithUTF8String:zipPath];
        NSFileManager* fileManager = [NSFileManager defaultManager];
        
        // Check if the file exists
        if (![fileManager fileExistsAtPath:zipPathStr]) {
            NSLog(@"Error: ZIP file does not exist: %@", zipPathStr);
            return NULL;
        }
        
        // Load the ZIP file into memory
        NSError* error = nil;
        NSData* zipData = [NSData dataWithContentsOfFile:zipPathStr options:NSDataReadingMappedIfSafe error:&error];
        
        if (!zipData) {
            NSLog(@"Error loading ZIP file: %@", error);
            return NULL;
        }
        
        // Extract the requested file from the ZIP data
        return ExtractFileFromZipData(zipData, fileName, fileSize);
    }
} 