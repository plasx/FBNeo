#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../metal_declarations.h"

// These are stub implementations of the zlib functions needed
// They don't actually do anything, but they allow the code to link

// Make stub functions visible to C++ code
#ifdef __cplusplus
extern "C" {
#endif

// Stub implementation of inflateInit2_
int inflateInit2_(void* strm, int windowBits, const char* version, int stream_size) {
    printf("STUB: inflateInit2_ called with windowBits=%d, version=%s, stream_size=%d\n",
           windowBits, version ? version : "NULL", stream_size);
    return 0; // Z_OK
}

// Stub implementation of inflate
int inflate(void* strm, int flush) {
    printf("STUB: inflate called with flush=%d\n", flush);
    return 1; // Z_STREAM_END
}

// Stub implementation of inflateEnd
int inflateEnd(void* strm) {
    printf("STUB: inflateEnd called\n");
    return 0; // Z_OK
}

#ifdef __cplusplus
}
#endif 