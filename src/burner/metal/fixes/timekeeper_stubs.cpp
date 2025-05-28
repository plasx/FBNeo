#include <string.h>
#include <stdlib.h>

// Basic types
typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int UINT32;
typedef int INT32;

// TimeKeeper stubs
extern "C" {
    void TimeKeeperExit() {
        // Stub implementation
    }
    
    int TimeKeeperInit(int type, UINT8* data) {
        // Stub implementation
        return 0;
    }
    
    UINT8 TimeKeeperRead(UINT32 offset) {
        // Stub implementation
        return 0;
    }
    
    void TimeKeeperTick() {
        // Stub implementation
    }
    
    void TimeKeeperWrite(int offset, UINT8 data) {
        // Stub implementation
    }
    
    int TimeKeeperScan(int nAction) {
        // Stub implementation
        return 0;
    }
} 