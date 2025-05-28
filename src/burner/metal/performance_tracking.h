#ifndef PERFORMANCE_TRACKING_H
#define PERFORMANCE_TRACKING_H

#include "metal_declarations.h"

#ifdef __cplusplus
extern "C" {
#endif

// Performance metric types
typedef enum {
    PERF_METRIC_FRAME_TIME,        // Frame rendering time in ms
    PERF_METRIC_CPU_USAGE,         // CPU usage percentage
    PERF_METRIC_GPU_USAGE,         // GPU usage percentage
    PERF_METRIC_MEMORY_USAGE,      // Memory usage in bytes
    PERF_METRIC_FPS,               // Frames per second
    PERF_METRIC_FRAME_PACING,      // Frame time variance (jitter)
    PERF_METRIC_AUDIO_UNDERRUNS,   // Audio buffer underruns
    PERF_METRIC_AUDIO_OVERRUNS,    // Audio buffer overruns
    PERF_METRIC_INPUT_LATENCY,     // Input to display latency in ms
    PERF_METRIC_CUSTOM,            // Custom metric
    PERF_METRIC_COUNT
} PerformanceMetricType;

// Performance counter structure
typedef struct {
    PerformanceMetricType type;    // Type of metric
    const char* name;              // Name of the metric
    float currentValue;            // Current value
    float minValue;                // Minimum observed value
    float maxValue;                // Maximum observed value
    float avgValue;                // Running average value
    int sampleCount;               // Number of samples taken
    float threshold;               // Threshold for warning/error
    int isWarning;                 // Flag indicating if current value exceeds warning threshold
    int isError;                   // Flag indicating if current value exceeds error threshold
} PerformanceCounter;

// Initialize performance tracking
void Performance_Init(void);

// Shutdown performance tracking
void Performance_Shutdown(void);

// Begin frame timing
void Performance_BeginFrame(void);

// End frame timing
void Performance_EndFrame(void);

// Update a specific metric
void Performance_UpdateMetric(PerformanceMetricType type, float value);

// Get the current value of a metric
float Performance_GetMetricValue(PerformanceMetricType type);

// Get performance counter structure
PerformanceCounter* Performance_GetCounter(PerformanceMetricType type);

// Set warning threshold for a metric
void Performance_SetWarningThreshold(PerformanceMetricType type, float threshold);

// Set error threshold for a metric
void Performance_SetErrorThreshold(PerformanceMetricType type, float threshold);

// Log all performance metrics
void Performance_LogMetrics(void);

// Enable/disable performance tracking
void Performance_SetEnabled(int enabled);

// Check if performance tracking is enabled
int Performance_IsEnabled(void);

// Reset all performance counters
void Performance_Reset(void);

// Create a custom performance metric
int Performance_CreateCustomMetric(const char* name);

// Update a custom metric
void Performance_UpdateCustomMetric(int customId, float value);

#ifdef __cplusplus
}
#endif

#endif // PERFORMANCE_TRACKING_H 