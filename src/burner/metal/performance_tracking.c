#include "performance_tracking.h"
#include "rom_loading_debug.h"
#include "memory_tracking.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

// Maximum number of custom metrics
#define MAX_CUSTOM_METRICS 16

// Performance counters
static PerformanceCounter g_counters[PERF_METRIC_COUNT + MAX_CUSTOM_METRICS];
static int g_numCustomMetrics = 0;
static int g_enabled = 0;

// Frame timing variables
static struct timespec g_frameStartTime;
static struct timespec g_lastFrameTime;
static float g_frameTimes[60] = {0}; // Last 60 frame times for average calculation
static int g_frameTimeIdx = 0;
static unsigned long long g_totalFrames = 0;

// Get time difference in milliseconds
static float GetTimeDiffMs(struct timespec* start, struct timespec* end) {
    return (end->tv_sec - start->tv_sec) * 1000.0f + 
           (end->tv_nsec - start->tv_nsec) / 1000000.0f;
}

// Calculate average of frame times
static float CalculateAverageFrameTime() {
    float sum = 0.0f;
    int count = 0;
    
    for (int i = 0; i < 60; i++) {
        if (g_frameTimes[i] > 0.0f) {
            sum += g_frameTimes[i];
            count++;
        }
    }
    
    return (count > 0) ? (sum / count) : 16.7f; // Default to 60fps (16.7ms) if no data
}

// Calculate variance (jitter) of frame times
static float CalculateFrameTimeVariance() {
    float avg = CalculateAverageFrameTime();
    float sumSqDiff = 0.0f;
    int count = 0;
    
    for (int i = 0; i < 60; i++) {
        if (g_frameTimes[i] > 0.0f) {
            float diff = g_frameTimes[i] - avg;
            sumSqDiff += diff * diff;
            count++;
        }
    }
    
    return (count > 1) ? sqrt(sumSqDiff / (count - 1)) : 0.0f;
}

// Initialize performance tracking
void Performance_Init(void) {
    // Initialize counters
    memset(g_counters, 0, sizeof(g_counters));
    
    // Set up default metrics
    g_counters[PERF_METRIC_FRAME_TIME].type = PERF_METRIC_FRAME_TIME;
    g_counters[PERF_METRIC_FRAME_TIME].name = "Frame Time (ms)";
    g_counters[PERF_METRIC_FRAME_TIME].minValue = 1000.0f; // Start with a high value
    g_counters[PERF_METRIC_FRAME_TIME].maxValue = 0.0f;    // Start with a low value
    g_counters[PERF_METRIC_FRAME_TIME].avgValue = 16.7f;   // Start with 60fps assumption
    
    g_counters[PERF_METRIC_CPU_USAGE].type = PERF_METRIC_CPU_USAGE;
    g_counters[PERF_METRIC_CPU_USAGE].name = "CPU Usage (%)";
    g_counters[PERF_METRIC_CPU_USAGE].minValue = 100.0f;
    g_counters[PERF_METRIC_CPU_USAGE].maxValue = 0.0f;
    
    g_counters[PERF_METRIC_GPU_USAGE].type = PERF_METRIC_GPU_USAGE;
    g_counters[PERF_METRIC_GPU_USAGE].name = "GPU Usage (%)";
    g_counters[PERF_METRIC_GPU_USAGE].minValue = 100.0f;
    g_counters[PERF_METRIC_GPU_USAGE].maxValue = 0.0f;
    
    g_counters[PERF_METRIC_MEMORY_USAGE].type = PERF_METRIC_MEMORY_USAGE;
    g_counters[PERF_METRIC_MEMORY_USAGE].name = "Memory Usage (MB)";
    g_counters[PERF_METRIC_MEMORY_USAGE].minValue = 1000000.0f;
    g_counters[PERF_METRIC_MEMORY_USAGE].maxValue = 0.0f;
    
    g_counters[PERF_METRIC_FPS].type = PERF_METRIC_FPS;
    g_counters[PERF_METRIC_FPS].name = "FPS";
    g_counters[PERF_METRIC_FPS].minValue = 1000.0f;
    g_counters[PERF_METRIC_FPS].maxValue = 0.0f;
    g_counters[PERF_METRIC_FPS].avgValue = 60.0f; // Start with 60fps assumption
    
    g_counters[PERF_METRIC_FRAME_PACING].type = PERF_METRIC_FRAME_PACING;
    g_counters[PERF_METRIC_FRAME_PACING].name = "Frame Pacing Variance (ms)";
    g_counters[PERF_METRIC_FRAME_PACING].minValue = 1000.0f;
    g_counters[PERF_METRIC_FRAME_PACING].maxValue = 0.0f;
    
    g_counters[PERF_METRIC_AUDIO_UNDERRUNS].type = PERF_METRIC_AUDIO_UNDERRUNS;
    g_counters[PERF_METRIC_AUDIO_UNDERRUNS].name = "Audio Underruns";
    
    g_counters[PERF_METRIC_AUDIO_OVERRUNS].type = PERF_METRIC_AUDIO_OVERRUNS;
    g_counters[PERF_METRIC_AUDIO_OVERRUNS].name = "Audio Overruns";
    
    g_counters[PERF_METRIC_INPUT_LATENCY].type = PERF_METRIC_INPUT_LATENCY;
    g_counters[PERF_METRIC_INPUT_LATENCY].name = "Input Latency (ms)";
    g_counters[PERF_METRIC_INPUT_LATENCY].minValue = 1000.0f;
    g_counters[PERF_METRIC_INPUT_LATENCY].maxValue = 0.0f;
    
    // Set default warning thresholds
    Performance_SetWarningThreshold(PERF_METRIC_FRAME_TIME, 20.0f);   // Warn if frame time exceeds 20ms (50fps)
    Performance_SetWarningThreshold(PERF_METRIC_CPU_USAGE, 80.0f);    // Warn if CPU usage exceeds 80%
    Performance_SetWarningThreshold(PERF_METRIC_FRAME_PACING, 5.0f);  // Warn if frame time variance exceeds 5ms
    
    // Set default error thresholds
    Performance_SetErrorThreshold(PERF_METRIC_FRAME_TIME, 33.3f);     // Error if frame time exceeds 33.3ms (30fps)
    Performance_SetErrorThreshold(PERF_METRIC_CPU_USAGE, 95.0f);      // Error if CPU usage exceeds 95%
    Performance_SetErrorThreshold(PERF_METRIC_FRAME_PACING, 10.0f);   // Error if frame time variance exceeds 10ms
    
    // Initialize timing
    clock_gettime(CLOCK_MONOTONIC, &g_lastFrameTime);
    g_frameTimeIdx = 0;
    g_totalFrames = 0;
    
    // Enable performance tracking
    g_enabled = 1;
    
    ROMLoader_TrackLoadStep("PERF INIT", "Performance tracking system initialized");
}

// Shutdown performance tracking
void Performance_Shutdown(void) {
    g_enabled = 0;
}

// Begin frame timing
void Performance_BeginFrame(void) {
    if (!g_enabled) return;
    
    clock_gettime(CLOCK_MONOTONIC, &g_frameStartTime);
}

// End frame timing
void Performance_EndFrame(void) {
    if (!g_enabled) return;
    
    struct timespec endTime;
    clock_gettime(CLOCK_MONOTONIC, &endTime);
    
    // Calculate frame time
    float frameTime = GetTimeDiffMs(&g_frameStartTime, &endTime);
    
    // Store in circular buffer
    g_frameTimes[g_frameTimeIdx] = frameTime;
    g_frameTimeIdx = (g_frameTimeIdx + 1) % 60;
    
    // Update frame time metric
    Performance_UpdateMetric(PERF_METRIC_FRAME_TIME, frameTime);
    
    // Update FPS metric (calculate from average frame time)
    float avgFrameTime = CalculateAverageFrameTime();
    float fps = (avgFrameTime > 0) ? (1000.0f / avgFrameTime) : 0.0f;
    Performance_UpdateMetric(PERF_METRIC_FPS, fps);
    
    // Update frame pacing metric
    float frameTimeVariance = CalculateFrameTimeVariance();
    Performance_UpdateMetric(PERF_METRIC_FRAME_PACING, frameTimeVariance);
    
    // Increment total frames
    g_totalFrames++;
    
    // Every 60 frames, log performance metrics
    if (g_totalFrames % 60 == 0) {
        Performance_LogMetrics();
    }
    
    // Update last frame time
    g_lastFrameTime = endTime;
}

// Update a specific metric
void Performance_UpdateMetric(PerformanceMetricType type, float value) {
    if (!g_enabled || type < 0 || type >= PERF_METRIC_COUNT) return;
    
    PerformanceCounter* counter = &g_counters[type];
    
    // Update current value
    counter->currentValue = value;
    
    // Update min/max
    if (value < counter->minValue || counter->sampleCount == 0) {
        counter->minValue = value;
    }
    if (value > counter->maxValue || counter->sampleCount == 0) {
        counter->maxValue = value;
    }
    
    // Update running average
    if (counter->sampleCount == 0) {
        counter->avgValue = value;
    } else {
        // Weighted average (more weight to recent values)
        counter->avgValue = counter->avgValue * 0.95f + value * 0.05f;
    }
    
    // Update sample count
    counter->sampleCount++;
    
    // Check thresholds
    counter->isWarning = (counter->threshold > 0 && value >= counter->threshold);
    counter->isError = (counter->threshold > 0 && value >= counter->threshold * 1.5f);
}

// Get the current value of a metric
float Performance_GetMetricValue(PerformanceMetricType type) {
    if (type < 0 || type >= PERF_METRIC_COUNT) return 0.0f;
    
    return g_counters[type].currentValue;
}

// Get performance counter structure
PerformanceCounter* Performance_GetCounter(PerformanceMetricType type) {
    if (type < 0 || type >= PERF_METRIC_COUNT) return NULL;
    
    return &g_counters[type];
}

// Set warning threshold for a metric
void Performance_SetWarningThreshold(PerformanceMetricType type, float threshold) {
    if (type < 0 || type >= PERF_METRIC_COUNT) return;
    
    g_counters[type].threshold = threshold;
}

// Set error threshold for a metric
void Performance_SetErrorThreshold(PerformanceMetricType type, float threshold) {
    if (type < 0 || type >= PERF_METRIC_COUNT) return;
    
    // Store as 1.5x the warning threshold
    g_counters[type].threshold = threshold / 1.5f;
}

// Log all performance metrics
void Performance_LogMetrics(void) {
    if (!g_enabled) return;
    
    // Log overall performance summary
    float fps = Performance_GetMetricValue(PERF_METRIC_FPS);
    float frameTime = Performance_GetMetricValue(PERF_METRIC_FRAME_TIME);
    float jitter = Performance_GetMetricValue(PERF_METRIC_FRAME_PACING);
    
    // Log always
    ROMLoader_TrackLoadStep("PERF LOOP", "Performance: %.1f FPS (%.2f ms/frame, %.2f ms jitter)",
                         fps, frameTime, jitter);
    
    // Log warnings
    for (int i = 0; i < PERF_METRIC_COUNT; i++) {
        if (g_counters[i].isWarning) {
            ROMLoader_DebugLog(LOG_WARNING, "Performance warning: %s = %.2f", 
                             g_counters[i].name, g_counters[i].currentValue);
        }
    }
    
    // Log detailed metrics in verbose mode
    ROMLoader_DebugLog(LOG_VERBOSE, "Performance metrics:");
    for (int i = 0; i < PERF_METRIC_COUNT; i++) {
        if (g_counters[i].sampleCount > 0) {
            ROMLoader_DebugLog(LOG_VERBOSE, "  %s: current=%.2f, avg=%.2f, min=%.2f, max=%.2f",
                             g_counters[i].name, g_counters[i].currentValue,
                             g_counters[i].avgValue, g_counters[i].minValue,
                             g_counters[i].maxValue);
        }
    }
    
    // Log custom metrics
    for (int i = 0; i < g_numCustomMetrics; i++) {
        PerformanceCounter* counter = &g_counters[PERF_METRIC_COUNT + i];
        if (counter->sampleCount > 0) {
            ROMLoader_DebugLog(LOG_VERBOSE, "  %s: current=%.2f, avg=%.2f, min=%.2f, max=%.2f",
                             counter->name, counter->currentValue,
                             counter->avgValue, counter->minValue,
                             counter->maxValue);
        }
    }
}

// Enable/disable performance tracking
void Performance_SetEnabled(int enabled) {
    g_enabled = enabled;
    
    if (enabled) {
        ROMLoader_DebugLog(LOG_INFO, "Performance tracking enabled");
    } else {
        ROMLoader_DebugLog(LOG_INFO, "Performance tracking disabled");
    }
}

// Check if performance tracking is enabled
int Performance_IsEnabled(void) {
    return g_enabled;
}

// Reset all performance counters
void Performance_Reset(void) {
    for (int i = 0; i < PERF_METRIC_COUNT + g_numCustomMetrics; i++) {
        g_counters[i].currentValue = 0.0f;
        g_counters[i].minValue = 1000000.0f;
        g_counters[i].maxValue = 0.0f;
        g_counters[i].avgValue = 0.0f;
        g_counters[i].sampleCount = 0;
    }
    
    // Reset frame time tracking
    g_frameTimeIdx = 0;
    memset(g_frameTimes, 0, sizeof(g_frameTimes));
    
    ROMLoader_DebugLog(LOG_INFO, "Performance counters reset");
}

// Create a custom performance metric
int Performance_CreateCustomMetric(const char* name) {
    if (!name || g_numCustomMetrics >= MAX_CUSTOM_METRICS) {
        return -1;
    }
    
    int id = PERF_METRIC_COUNT + g_numCustomMetrics;
    
    // Initialize custom metric
    PerformanceCounter* counter = &g_counters[id];
    counter->type = PERF_METRIC_CUSTOM;
    counter->name = strdup(name);
    counter->minValue = 1000000.0f;
    counter->maxValue = 0.0f;
    counter->avgValue = 0.0f;
    counter->sampleCount = 0;
    
    g_numCustomMetrics++;
    
    ROMLoader_DebugLog(LOG_INFO, "Created custom performance metric: %s (id=%d)", 
                     name, id - PERF_METRIC_COUNT);
    
    return id - PERF_METRIC_COUNT;
}

// Update a custom metric
void Performance_UpdateCustomMetric(int customId, float value) {
    int id = PERF_METRIC_COUNT + customId;
    
    if (!g_enabled || customId < 0 || customId >= g_numCustomMetrics) {
        return;
    }
    
    // Update the metric
    Performance_UpdateMetric(id, value);
} 