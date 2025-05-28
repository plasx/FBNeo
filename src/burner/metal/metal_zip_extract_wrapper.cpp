#include <cstdio>
#include <cstdlib>

// Include our headers
#include "metal_declarations.h"
#include "metal_zip_extract.h"
#include "rom_loading_debug.h"

// These function prototypes must exactly match the ones used in the obj/metal_zip_extract.o file
extern "C" {
    int Metal_ExtractFileFromZip(const char* zipPath, const char* filename, UINT8* buffer, int bufferSize, int* bytesExtracted);
    int Metal_GetZipFileInfo(const char* zipPath, const char* filename, UINT32* size, UINT32* crc);
    int Metal_ListZipContents(const char* zipPath, char** filenames, int maxFiles, int* numFiles);
}

// We don't need to implement the functions as they exist in the .o file
// We just need to ensure the proper linkage between them 