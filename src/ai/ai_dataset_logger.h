#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <vector>
#include <queue>
#include <thread>
#include <atomic>
#include <condition_variable>
#include "ai_controller.h"

/**
 * AIDatasetLogger - Records gameplay data (observations and actions) to JSONL files
 * 
 * Features:
 * - Thread-safe logging via worker thread
 * - File rotation when size exceeds threshold
 * - Optional compression of rotated files
 * - Buffered writing for performance
 */
class AIDatasetLogger {
public:
    /**
     * Constructor
     * 
     * @param outputDir Directory to store log files
     * @param baseFilename Base name for log files (will be appended with timestamp)
     * @param maxFileSize Maximum file size in bytes before rotation (default: 100MB)
     * @param useCompression Whether to compress rotated files (default: true)
     */
    AIDatasetLogger(const std::string& outputDir, 
                   const std::string& baseFilename,
                   size_t maxFileSize = 100 * 1024 * 1024,  // 100MB default
                   bool useCompression = true);
    
    /**
     * Destructor - ensures all pending logs are written and resources cleaned up
     */
    ~AIDatasetLogger();
    
    /**
     * Log a frame of gameplay data
     * 
     * @param observation Current game observation
     * @param action Action taken in response to observation
     * @param frameNumber The current frame number
     * @param reward Optional reward value
     * @return true if successfully queued for logging, false otherwise
     */
    bool logFrame(const GameObservation& observation, 
                 const InputAction& action,
                 uint64_t frameNumber,
                 float reward = 0.0f);
    
    /**
     * Flush all pending logs to disk immediately
     */
    void flush();
    
    /**
     * Start a new log file (rotates current file if it exists)
     * 
     * @return true if successful, false otherwise
     */
    bool startNewLogFile();
    
    /**
     * Enable or disable logging
     * 
     * @param enabled Whether logging should be enabled
     */
    void setEnabled(bool enabled);
    
    /**
     * Check if logger is currently enabled
     * 
     * @return true if enabled, false otherwise
     */
    bool isEnabled() const;

private:
    // Logger state
    std::string m_outputDir;
    std::string m_baseFilename;
    std::string m_currentFilename;
    size_t m_maxFileSize;
    bool m_useCompression;
    std::ofstream m_outputFile;
    std::atomic<bool> m_enabled;
    std::atomic<bool> m_shutdown;
    
    // Thread synchronization
    std::mutex m_queueMutex;
    std::condition_variable m_queueCV;
    std::queue<std::string> m_logQueue;
    std::thread m_workerThread;
    
    // Worker thread function
    void workerThreadFunc();
    
    // Utility functions
    std::string generateTimestamp();
    std::string formatLogEntry(const GameObservation& observation, 
                              const InputAction& action,
                              uint64_t frameNumber,
                              float reward);
    bool rotateLogFile();
    bool compressFile(const std::string& filename);
};

// Global instance
extern AIDatasetLogger* g_pAIDatasetLogger; 