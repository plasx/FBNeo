#ifndef METAL_ZIP_EXTRACT_H
#define METAL_ZIP_EXTRACT_H

#include "metal_declarations.h"

#ifdef __cplusplus
extern "C" {
#endif

// Extract a file from a ZIP archive
// Returns 0 on success, non-zero on failure
// buffer is allocated by the function and must be freed by the caller
int Metal_ExtractFileFromZip(const char* zipPath, const char* fileName, UINT8*& buffer, UINT32 expectedSize, int* actualSize);

// List contents of a ZIP file
// filenames is an array of char* that must be pre-allocated
// Returns 0 on success, non-zero on failure
int Metal_ListZipContents(const char* zipPath, char** filenames, int maxFiles, int* numFiles);

// Get information about a file in a ZIP
// Returns 0 on success, non-zero on failure
int Metal_GetZipFileInfo(const char* zipPath, const char* fileName, UINT32* fileSize, UINT32* crc32);

#ifdef __cplusplus
}
#endif

#endif // METAL_ZIP_EXTRACT_H 