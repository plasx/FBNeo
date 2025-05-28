#ifndef _METAL_PERFORMANCE_COUNTERS_H_
#define _METAL_PERFORMANCE_COUNTERS_H_

// ======================================================================
// Metal Performance Counters Interface
// ======================================================================
// This header defines the interface for the Metal performance counter
// system used for tracking GPU and CPU usage, frame times, and other
// metrics for debugging and optimization.
// ======================================================================

#ifdef __cplusplus
extern "C" {
#endif

// Initialize performance counters
void Metal_InitPerformanceCounters();

// Shutdown performance counters
void Metal_ShutdownPerformanceCounters();

// Enable or disable performance counters
void Metal_SetPerformanceCountersEnabled(int enabled);

// Enable or disable detailed logging (creates CSV output)
void Metal_SetDetailedLoggingEnabled(int enabled);

// Begin collecting metrics for a frame
void Metal_BeginFrameMetrics();

// Record a Metal command buffer for GPU timing
void Metal_RecordCommandBuffer(void* commandBuffer);

// End metrics collection for a frame
void Metal_EndFrameMetrics();

// Add draw call count for current frame
void Metal_AddDrawCallCount(int count);

// Add triangle count for current frame
void Metal_AddTriangleCount(int count);

// Add texture memory usage for current frame
void Metal_AddTextureMemoryUsage(size_t bytes);

// Add buffer memory usage for current frame
void Metal_AddBufferMemoryUsage(size_t bytes);

// Dump current performance counter values to log
void Metal_DumpPerformanceCounters();

// Get current FPS
float Metal_GetCurrentFPS();

// Get current CPU time in milliseconds
float Metal_GetCPUTimeMs();

// Get current GPU time in milliseconds
float Metal_GetGPUTimeMs();

// Get performance metrics
void Metal_GetPerformanceMetrics(float* fps, float* cpuTime, float* gpuTime, float* frameTime);

#ifdef __cplusplus
}
#endif

#endif // _METAL_PERFORMANCE_COUNTERS_H_ 