#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <QuartzCore/CAMetalLayer.h>

// Include the performance counter interface
#include "metal_performance_counters.h"

// ======================================================================
// Metal Performance Counters Implementation
// ======================================================================
// This file implements performance counters for tracking GPU and CPU
// usage, frame times, and other metrics for debugging and optimization.
// ======================================================================

// Maximum number of samples to keep in the history
#define MAX_SAMPLES 120 // 2 seconds at 60fps

// Structure to hold a single frame's performance metrics
typedef struct {
    uint64_t frameNumber;
    double frameStartTime;
    double frameEndTime;
    double gpuStartTime;
    double gpuEndTime;
    double cpuTimeMs;
    double gpuTimeMs;
    double totalFrameTimeMs;
    size_t drawCallCount;
    size_t triangleCount;
    size_t textureMemoryUsed;
    size_t bufferMemoryUsed;
} FrameMetrics;

// Performance counter state
static struct {
    bool enabled;
    bool detailedLogging;
    bool initialized;
    
    // Metrics history
    FrameMetrics history[MAX_SAMPLES];
    int historyIndex;
    int historySamples;
    
    // Current frame metrics
    FrameMetrics currentFrame;
    
    // Global statistics
    uint64_t totalFrames;
    double peakCpuTimeMs;
    double peakGpuTimeMs;
    double peakFrameTimeMs;
    double averageCpuTimeMs;
    double averageGpuTimeMs;
    double averageFrameTimeMs;
    
    // Metal performance counters
    MTLCaptureManager* captureManager;
    id<MTLCaptureScope> frameScope;
    id<MTLCommandBuffer> currentCommandBuffer;
    
    // Timers
    CFTimeInterval lastFrameTime;
    dispatch_semaphore_t frameSemaphore;
    
    // Buffer for CSV logging
    NSMutableString* csvBuffer;
    BOOL csvHeaderWritten;
    
} perfCounters;

// Initialize the performance counters
extern "C" void Metal_InitPerformanceCounters() {
    if (perfCounters.initialized) {
        return;
    }
    
    // Reset all metrics
    memset(&perfCounters, 0, sizeof(perfCounters));
    
    perfCounters.enabled = true;
    perfCounters.detailedLogging = false;
    perfCounters.historyIndex = 0;
    perfCounters.historySamples = 0;
    perfCounters.totalFrames = 0;
    
    // Initialize Metal capture if available
    perfCounters.captureManager = [MTLCaptureManager sharedCaptureManager];
    if (perfCounters.captureManager) {
        perfCounters.frameScope = [perfCounters.captureManager newCaptureScopeWithDevice:MTLCreateSystemDefaultDevice()];
        [perfCounters.frameScope setLabel:@"FBNeo Frame Capture"];
    }
    
    // Initialize timer
    perfCounters.lastFrameTime = CACurrentMediaTime();
    perfCounters.frameSemaphore = dispatch_semaphore_create(1);
    
    // Initialize CSV buffer
    perfCounters.csvBuffer = [NSMutableString stringWithCapacity:4096];
    perfCounters.csvHeaderWritten = NO;
    
    perfCounters.initialized = true;
    
    NSLog(@"Metal performance counters initialized");
}

// Shutdown the performance counters
extern "C" void Metal_ShutdownPerformanceCounters() {
    if (!perfCounters.initialized) {
        return;
    }
    
    // Dump final stats
    Metal_DumpPerformanceCounters();
    
    // Release objects
    perfCounters.frameScope = nil;
    perfCounters.captureManager = nil;
    perfCounters.currentCommandBuffer = nil;
    perfCounters.frameSemaphore = nil;
    
    // Write CSV buffer to file if detailed logging was enabled
    if (perfCounters.detailedLogging && [perfCounters.csvBuffer length] > 0) {
        NSString* documentsPath = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) firstObject];
        NSString* filePath = [documentsPath stringByAppendingPathComponent:@"FBNeo_PerformanceLog.csv"];
        
        NSError* error = nil;
        BOOL success = [perfCounters.csvBuffer writeToFile:filePath atomically:YES encoding:NSUTF8StringEncoding error:&error];
        
        if (success) {
            NSLog(@"Performance log written to: %@", filePath);
        } else {
            NSLog(@"Failed to write performance log: %@", error);
        }
    }
    
    perfCounters.csvBuffer = nil;
    perfCounters.initialized = false;
    
    NSLog(@"Metal performance counters shutdown");
}

// Enable or disable performance counters
extern "C" void Metal_SetPerformanceCountersEnabled(int enabled) {
    perfCounters.enabled = (enabled != 0);
}

// Enable or disable detailed logging
extern "C" void Metal_SetDetailedLoggingEnabled(int enabled) {
    perfCounters.detailedLogging = (enabled != 0);
    
    // If enabling detailed logging and CSV header not written, write it
    if (perfCounters.detailedLogging && !perfCounters.csvHeaderWritten) {
        [perfCounters.csvBuffer appendString:@"Frame,CPUTime,GPUTime,TotalTime,DrawCalls,Triangles,TextureMemory,BufferMemory\n"];
        perfCounters.csvHeaderWritten = YES;
    }
}

// Begin frame metrics collection
extern "C" void Metal_BeginFrameMetrics() {
    if (!perfCounters.initialized || !perfCounters.enabled) {
        return;
    }
    
    // Reset current frame metrics
    memset(&perfCounters.currentFrame, 0, sizeof(FrameMetrics));
    
    // Set frame number
    perfCounters.currentFrame.frameNumber = perfCounters.totalFrames;
    
    // Record start time
    perfCounters.currentFrame.frameStartTime = CACurrentMediaTime();
    
    // Begin Metal capture if detailed logging enabled
    if (perfCounters.detailedLogging && perfCounters.frameScope) {
        [perfCounters.frameScope beginScope];
    }
}

// Record a Metal command buffer for GPU timing
extern "C" void Metal_RecordCommandBuffer(void* commandBuffer) {
    if (!perfCounters.initialized || !perfCounters.enabled) {
        return;
    }
    
    // Store command buffer for timing
    perfCounters.currentCommandBuffer = (__bridge id<MTLCommandBuffer>)commandBuffer;
    
    // Set GPU start time when the command buffer starts executing
    if (perfCounters.currentCommandBuffer) {
        [perfCounters.currentCommandBuffer addScheduledHandler:^(id<MTLCommandBuffer> buffer) {
            if (perfCounters.initialized && perfCounters.enabled) {
                // Use a semaphore to ensure thread safety
                dispatch_semaphore_wait(perfCounters.frameSemaphore, DISPATCH_TIME_FOREVER);
                perfCounters.currentFrame.gpuStartTime = CACurrentMediaTime();
                dispatch_semaphore_signal(perfCounters.frameSemaphore);
            }
        }];
        
        // Set GPU end time when the command buffer completes
        [perfCounters.currentCommandBuffer addCompletedHandler:^(id<MTLCommandBuffer> buffer) {
            if (perfCounters.initialized && perfCounters.enabled) {
                // Use a semaphore to ensure thread safety
                dispatch_semaphore_wait(perfCounters.frameSemaphore, DISPATCH_TIME_FOREVER);
                perfCounters.currentFrame.gpuEndTime = CACurrentMediaTime();
                
                // Calculate GPU time in milliseconds
                perfCounters.currentFrame.gpuTimeMs = 
                    (perfCounters.currentFrame.gpuEndTime - perfCounters.currentFrame.gpuStartTime) * 1000.0;
                
                // Update peak GPU time
                if (perfCounters.currentFrame.gpuTimeMs > perfCounters.peakGpuTimeMs) {
                    perfCounters.peakGpuTimeMs = perfCounters.currentFrame.gpuTimeMs;
                }
                
                dispatch_semaphore_signal(perfCounters.frameSemaphore);
            }
        }];
    }
}

// End frame metrics collection
extern "C" void Metal_EndFrameMetrics() {
    if (!perfCounters.initialized || !perfCounters.enabled) {
        return;
    }
    
    // Record end time
    perfCounters.currentFrame.frameEndTime = CACurrentMediaTime();
    
    // Calculate CPU time in milliseconds
    perfCounters.currentFrame.cpuTimeMs = 
        (perfCounters.currentFrame.frameEndTime - perfCounters.currentFrame.frameStartTime) * 1000.0;
    
    // Calculate total frame time in milliseconds
    perfCounters.currentFrame.totalFrameTimeMs = 
        (perfCounters.currentFrame.frameEndTime - perfCounters.lastFrameTime) * 1000.0;
    
    // Update last frame time
    perfCounters.lastFrameTime = perfCounters.currentFrame.frameEndTime;
    
    // End Metal capture if detailed logging enabled
    if (perfCounters.detailedLogging && perfCounters.frameScope) {
        [perfCounters.frameScope endScope];
    }
    
    // Update peak times
    if (perfCounters.currentFrame.cpuTimeMs > perfCounters.peakCpuTimeMs) {
        perfCounters.peakCpuTimeMs = perfCounters.currentFrame.cpuTimeMs;
    }
    
    if (perfCounters.currentFrame.totalFrameTimeMs > perfCounters.peakFrameTimeMs) {
        perfCounters.peakFrameTimeMs = perfCounters.currentFrame.totalFrameTimeMs;
    }
    
    // Update averages
    perfCounters.totalFrames++;
    perfCounters.averageCpuTimeMs = 
        (perfCounters.averageCpuTimeMs * (perfCounters.totalFrames - 1) + perfCounters.currentFrame.cpuTimeMs) / 
        perfCounters.totalFrames;
    
    perfCounters.averageGpuTimeMs = 
        (perfCounters.averageGpuTimeMs * (perfCounters.totalFrames - 1) + perfCounters.currentFrame.gpuTimeMs) / 
        perfCounters.totalFrames;
    
    perfCounters.averageFrameTimeMs = 
        (perfCounters.averageFrameTimeMs * (perfCounters.totalFrames - 1) + perfCounters.currentFrame.totalFrameTimeMs) / 
        perfCounters.totalFrames;
    
    // Add to history
    perfCounters.history[perfCounters.historyIndex] = perfCounters.currentFrame;
    perfCounters.historyIndex = (perfCounters.historyIndex + 1) % MAX_SAMPLES;
    if (perfCounters.historySamples < MAX_SAMPLES) {
        perfCounters.historySamples++;
    }
    
    // Add to CSV log if detailed logging is enabled
    if (perfCounters.detailedLogging) {
        [perfCounters.csvBuffer appendFormat:@"%llu,%.2f,%.2f,%.2f,%zu,%zu,%zu,%zu\n",
            perfCounters.currentFrame.frameNumber,
            perfCounters.currentFrame.cpuTimeMs,
            perfCounters.currentFrame.gpuTimeMs,
            perfCounters.currentFrame.totalFrameTimeMs,
            perfCounters.currentFrame.drawCallCount,
            perfCounters.currentFrame.triangleCount,
            perfCounters.currentFrame.textureMemoryUsed,
            perfCounters.currentFrame.bufferMemoryUsed];
    }
}

// Add draw call count
extern "C" void Metal_AddDrawCallCount(int count) {
    if (!perfCounters.initialized || !perfCounters.enabled) {
        return;
    }
    
    perfCounters.currentFrame.drawCallCount += count;
}

// Add triangle count
extern "C" void Metal_AddTriangleCount(int count) {
    if (!perfCounters.initialized || !perfCounters.enabled) {
        return;
    }
    
    perfCounters.currentFrame.triangleCount += count;
}

// Add texture memory usage
extern "C" void Metal_AddTextureMemoryUsage(size_t bytes) {
    if (!perfCounters.initialized || !perfCounters.enabled) {
        return;
    }
    
    perfCounters.currentFrame.textureMemoryUsed += bytes;
}

// Add buffer memory usage
extern "C" void Metal_AddBufferMemoryUsage(size_t bytes) {
    if (!perfCounters.initialized || !perfCounters.enabled) {
        return;
    }
    
    perfCounters.currentFrame.bufferMemoryUsed += bytes;
}

// Dump current performance counters
extern "C" void Metal_DumpPerformanceCounters() {
    if (!perfCounters.initialized) {
        return;
    }
    
    NSLog(@"========== Metal Performance Counters ==========");
    NSLog(@"Total Frames: %llu", perfCounters.totalFrames);
    NSLog(@"CPU Time:     %.2f ms (avg), %.2f ms (peak)", 
         perfCounters.averageCpuTimeMs, perfCounters.peakCpuTimeMs);
    NSLog(@"GPU Time:     %.2f ms (avg), %.2f ms (peak)", 
         perfCounters.averageGpuTimeMs, perfCounters.peakGpuTimeMs);
    NSLog(@"Frame Time:   %.2f ms (avg), %.2f ms (peak)", 
         perfCounters.averageFrameTimeMs, perfCounters.peakFrameTimeMs);
    NSLog(@"FPS:          %.1f (avg), %.1f (min)", 
         1000.0 / perfCounters.averageFrameTimeMs, 
         1000.0 / perfCounters.peakFrameTimeMs);
    
    // Only show these if we have actual data
    if (perfCounters.currentFrame.drawCallCount > 0) {
        NSLog(@"Draw Calls:   %zu", perfCounters.currentFrame.drawCallCount);
        NSLog(@"Triangles:    %zu", perfCounters.currentFrame.triangleCount);
    }
    
    if (perfCounters.currentFrame.textureMemoryUsed > 0 || perfCounters.currentFrame.bufferMemoryUsed > 0) {
        NSLog(@"Texture Mem:  %.2f MB", perfCounters.currentFrame.textureMemoryUsed / (1024.0 * 1024.0));
        NSLog(@"Buffer Mem:   %.2f MB", perfCounters.currentFrame.bufferMemoryUsed / (1024.0 * 1024.0));
    }
    
    NSLog(@"==============================================");
}

// Get current FPS
extern "C" float Metal_GetCurrentFPS() {
    if (!perfCounters.initialized || perfCounters.averageFrameTimeMs <= 0) {
        return 60.0f;
    }
    
    return (float)(1000.0 / perfCounters.averageFrameTimeMs);
}

// Get current CPU time in milliseconds
extern "C" float Metal_GetCPUTimeMs() {
    if (!perfCounters.initialized) {
        return 0.0f;
    }
    
    return (float)perfCounters.averageCpuTimeMs;
}

// Get current GPU time in milliseconds
extern "C" float Metal_GetGPUTimeMs() {
    if (!perfCounters.initialized) {
        return 0.0f;
    }
    
    return (float)perfCounters.averageGpuTimeMs;
}

// Get performance metrics
extern "C" void Metal_GetPerformanceMetrics(float* fps, float* cpuTime, float* gpuTime, float* frameTime) {
    if (!perfCounters.initialized) {
        if (fps) *fps = 60.0f;
        if (cpuTime) *cpuTime = 0.0f;
        if (gpuTime) *gpuTime = 0.0f;
        if (frameTime) *frameTime = 16.67f;
        return;
    }
    
    if (fps) *fps = (float)(1000.0 / perfCounters.averageFrameTimeMs);
    if (cpuTime) *cpuTime = (float)perfCounters.averageCpuTimeMs;
    if (gpuTime) *gpuTime = (float)perfCounters.averageGpuTimeMs;
    if (frameTime) *frameTime = (float)perfCounters.averageFrameTimeMs;
} 