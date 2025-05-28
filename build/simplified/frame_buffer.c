#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

typedef struct {
    uint32_t* data;
    int width;
    int height;
    int pitch;
    bool updated;
} FrameBuffer;

FrameBuffer g_frameBuffer = {0};

bool InitFrameBuffer(int width, int height) {
    if (g_frameBuffer.data) {
        free(g_frameBuffer.data);
    }
    
    g_frameBuffer.data = malloc(width * height * sizeof(uint32_t));
    if (!g_frameBuffer.data) {
        printf("Failed to allocate frame buffer\n");
        return false;
    }
    
    g_frameBuffer.width = width;
    g_frameBuffer.height = height;
    g_frameBuffer.pitch = width * sizeof(uint32_t);
    memset(g_frameBuffer.data, 0, width * height * sizeof(uint32_t));
    
    return true;
}

int Metal_GenerateTestPattern(int patternType) {
    int width = g_frameBuffer.width;
    int height = g_frameBuffer.height;
    
    if (!g_frameBuffer.data) {
        return -1;
    }
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            uint32_t color = ((x / 16) + (y / 16)) % 2 ? 0xFFFFFFFF : 0xFF000000;
            g_frameBuffer.data[y * width + x] = color;
        }
    }
    
    g_frameBuffer.updated = true;
    return 0;
}
