#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Type definitions
typedef int INT32;
typedef unsigned int UINT32;
typedef unsigned char UINT8;

// Structure used for area scanning
struct BurnArea { 
    void *Data; 
    UINT32 nLen; 
    INT32 nAddress; 
    char *szName; 
};

// Create stubs for the Burn functions that are not available in our minimal build
extern "C" {
    // Callback function pointer
    INT32 (*BurnAcb)(struct BurnArea* pba) = NULL;
    
    // Stub implementation of BurnAreaScan
    INT32 BurnAreaScan(INT32 nAction, INT32* pnMin) {
        printf("[BurnAreaScan] Called with nAction=%d (stub implementation)\n", nAction);
        
        // Allocate a much larger dummy area for CPS2 game state simulation
        // CPS2 games like Marvel vs Capcom have larger state requirements
        // This is still a stub, but at least the size is more realistic
        static UINT8 dummyData[2 * 1024 * 1024]; // 2MB
        static char dummyName[] = "CPS2GameState";
        
        struct BurnArea dummyArea;
        dummyArea.Data = dummyData;
        dummyArea.nLen = sizeof(dummyData);
        dummyArea.nAddress = 0;
        dummyArea.szName = dummyName;
        
        // If the callback is set, call it with our dummy area
        if (BurnAcb) {
            BurnAcb(&dummyArea);
        }
        
        // Also add some dummy subsystems to mimic a real CPS2 state
        if (BurnAcb) {
            // Add CPU state
            static UINT8 cpuData[64 * 1024]; // 64KB
            static char cpuName[] = "CPU_State";
            
            struct BurnArea cpuArea;
            cpuArea.Data = cpuData;
            cpuArea.nLen = sizeof(cpuData);
            cpuArea.nAddress = 0;
            cpuArea.szName = cpuName;
            
            BurnAcb(&cpuArea);
            
            // Add RAM state
            static UINT8 ramData[256 * 1024]; // 256KB
            static char ramName[] = "MainRAM";
            
            struct BurnArea ramArea;
            ramArea.Data = ramData;
            ramArea.nLen = sizeof(ramData);
            ramArea.nAddress = 0;
            ramArea.szName = ramName;
            
            BurnAcb(&ramArea);
            
            // Add VRAM state
            static UINT8 vramData[512 * 1024]; // 512KB
            static char vramName[] = "VideoRAM";
            
            struct BurnArea vramArea;
            vramArea.Data = vramData;
            vramArea.nLen = sizeof(vramData);
            vramArea.nAddress = 0;
            vramArea.szName = vramName;
            
            BurnAcb(&vramArea);
            
            // Add CPS2 specific state
            static UINT8 cpsData[128 * 1024]; // 128KB 
            static char cpsName[] = "CPS2_Registers";
            
            struct BurnArea cpsArea;
            cpsArea.Data = cpsData;
            cpsArea.nLen = sizeof(cpsData);
            cpsArea.nAddress = 0;
            cpsArea.szName = cpsName;
            
            BurnAcb(&cpsArea);
        }
        
        return 0;
    }
}

// State scan action flags
#define ACB_READ       (1<<0)
#define ACB_WRITE      (1<<1)
#define ACB_FULLSCAN   (0x0F << 3)

// Structure to calculate size
struct CalculateSizeData {
    INT32 totalSize;
    INT32 numEntries;
};

// Global state data
static CalculateSizeData g_sizeData = {0, 0};

// Function to calculate total size
static INT32 CalculateSizeCallback(struct BurnArea* pba) {
    if (pba && pba->Data && pba->nLen > 0) {
        g_sizeData.totalSize += pba->nLen;
        g_sizeData.numEntries++;
        printf("[BurnStateCompress] Area: %s, Size: %d bytes\n", pba->szName, pba->nLen);
    }
    return 0;
}

// Structure for save data
struct SaveStateData {
    UINT8* buffer;
    INT32 currentOffset;
    INT32 totalSize;
};

// Global save data
static SaveStateData g_saveData = {NULL, 0, 0};

// Function to save data
static INT32 SaveDataCallback(struct BurnArea* pba) {
    if (!pba || !pba->Data || pba->nLen <= 0) {
        return 0;
    }
    
    // Calculate offset after name and size info
    INT32 nameLen = strlen(pba->szName) + 1; // Include null terminator
    INT32 infoSize = sizeof(INT32) + nameLen;
    
    // Check if we have enough space
    if (g_saveData.currentOffset + infoSize + pba->nLen > g_saveData.totalSize) {
        printf("[BurnStateCompress] Warning: Buffer overflow\n");
        return 1;
    }
    
    // Write name length
    *(INT32*)(g_saveData.buffer + g_saveData.currentOffset) = nameLen;
    g_saveData.currentOffset += sizeof(INT32);
    
    // Write name
    memcpy(g_saveData.buffer + g_saveData.currentOffset, pba->szName, nameLen);
    g_saveData.currentOffset += nameLen;
    
    // Write data length
    *(INT32*)(g_saveData.buffer + g_saveData.currentOffset) = pba->nLen;
    g_saveData.currentOffset += sizeof(INT32);
    
    // Write data
    memcpy(g_saveData.buffer + g_saveData.currentOffset, pba->Data, pba->nLen);
    g_saveData.currentOffset += pba->nLen;
    
    return 0;
}

// State header structure
struct StateHeader {
    UINT32 magic;
    UINT32 numEntries;
    UINT32 dataSize;
};

// Structure for load data
struct LoadStateData {
    UINT8* buffer;
    INT32 currentOffset;
    INT32 totalSize;
};

// Global load data
static LoadStateData g_loadData = {NULL, 0, 0};

// Function to load data
static INT32 LoadDataCallback(struct BurnArea* pba) {
    if (!pba || !pba->Data || pba->nLen <= 0) {
        return 0;
    }
    
    // Search for matching area in the state data
    INT32 offset = sizeof(StateHeader);
    
    while (offset < g_loadData.totalSize) {
        // Read name length
        INT32 nameLen = *(INT32*)(g_loadData.buffer + offset);
        offset += sizeof(INT32);
        
        if (offset + nameLen >= g_loadData.totalSize) {
            break;
        }
        
        // Read name
        char* name = (char*)(g_loadData.buffer + offset);
        offset += nameLen;
        
        // Read data length
        INT32 dataLen = *(INT32*)(g_loadData.buffer + offset);
        offset += sizeof(INT32);
        
        if (offset + dataLen > g_loadData.totalSize) {
            break;
        }
        
        // Check if this is the area we're looking for
        if (strcmp(name, pba->szName) == 0) {
            // Found it - copy the data (limited to available space)
            INT32 copyLen = (dataLen < pba->nLen) ? dataLen : pba->nLen;
            memcpy(pba->Data, g_loadData.buffer + offset, copyLen);
            
            printf("[BurnStateDecompress] Loaded area: %s (%d bytes)\n", pba->szName, copyLen);
            return 0;
        }
        
        // Skip this entry
        offset += dataLen;
    }
    
    // Area not found
    printf("[BurnStateDecompress] Warning: Area not found: %s\n", pba->szName);
    return 0;
}

// Magic identifier for state files
#define STATE_MAGIC 0x46424E53  // "FBNS"

// Simple state compression/decompression
extern "C" INT32 BurnStateCompress(UINT8** pDef, INT32* pnDefLen, INT32 bAll) {
    printf("[BurnStateCompress] Compressing state data (bAll=%d)\n", bAll);
    
    // Initialize size data
    g_sizeData.totalSize = 0;
    g_sizeData.numEntries = 0;
    
    // Save original callback pointer
    INT32 (*oldAcb)(struct BurnArea* pba) = BurnAcb;
    
    // Set our callback
    BurnAcb = CalculateSizeCallback;
    
    // Scan with read flag to calculate size
    INT32 scanAction = ACB_READ;
    if (bAll) {
        scanAction |= ACB_FULLSCAN;
    }
    
    BurnAreaScan(scanAction, NULL);
    
    // Reserve 1KB overhead for header info
    INT32 totalSize = g_sizeData.totalSize + 1024;
    printf("[BurnStateCompress] Total state size: %d bytes (%d entries)\n", 
           totalSize, g_sizeData.numEntries);
    
    // Allocate buffer for state data
    UINT8* stateBuffer = (UINT8*)malloc(totalSize);
    if (!stateBuffer) {
        printf("[BurnStateCompress] Failed to allocate memory for state buffer\n");
        BurnAcb = oldAcb;
        return 1;
    }
    
    // Clear buffer
    memset(stateBuffer, 0, totalSize);
    
    // Write header
    StateHeader* header = (StateHeader*)stateBuffer;
    header->magic = STATE_MAGIC;
    header->numEntries = g_sizeData.numEntries;
    header->dataSize = g_sizeData.totalSize;
    
    // Initialize save data
    g_saveData.buffer = stateBuffer;
    g_saveData.currentOffset = sizeof(StateHeader);
    g_saveData.totalSize = totalSize;
    
    // Second pass - save data
    BurnAcb = SaveDataCallback;
    BurnAreaScan(scanAction, NULL);
    
    // Restore original callback
    BurnAcb = oldAcb;
    
    // Return the buffer and size
    if (pDef) *pDef = stateBuffer;
    if (pnDefLen) *pnDefLen = g_saveData.currentOffset;
    
    printf("[BurnStateCompress] State compressed successfully (%d bytes)\n", g_saveData.currentOffset);
    
    return 0;
}

// Decompress state data
extern "C" INT32 BurnStateDecompress(UINT8* Def, INT32 nDefLen, INT32 bAll) {
    if (!Def || nDefLen <= 0) {
        printf("[BurnStateDecompress] Invalid state data\n");
        return 1;
    }
    
    printf("[BurnStateDecompress] Decompressing state data (bAll=%d, size=%d)\n", bAll, nDefLen);
    
    // Validate header
    StateHeader* header = (StateHeader*)Def;
    if (header->magic != STATE_MAGIC) {
        printf("[BurnStateDecompress] Invalid state data (bad magic)\n");
        return 1;
    }
    
    printf("[BurnStateDecompress] State info: %d entries, %d bytes data\n", 
           header->numEntries, header->dataSize);
    
    // Initialize load data
    g_loadData.buffer = Def;
    g_loadData.currentOffset = sizeof(StateHeader);
    g_loadData.totalSize = nDefLen;
    
    // Save original callback pointer
    INT32 (*oldAcb)(struct BurnArea* pba) = BurnAcb;
    
    // Set our callback
    BurnAcb = LoadDataCallback;
    
    // Scan with write flag to load data
    INT32 scanAction = ACB_WRITE;
    if (bAll) {
        scanAction |= ACB_FULLSCAN;
    }
    
    BurnAreaScan(scanAction, NULL);
    
    // Restore original callback
    BurnAcb = oldAcb;
    
    printf("[BurnStateDecompress] State decompressed successfully\n");
    
    return 0;
} 