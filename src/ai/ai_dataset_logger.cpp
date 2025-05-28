#include "ai_dataset_logger.h"
#include "ai_input_frame.h"
#include "ai_output_action.h"
#include <iostream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <filesystem>
#include <nlohmann/json.hpp>

// For compression
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace AI {

AIDatasetLogger::AIDatasetLogger(
    const std::string& outputDir,
    const std::string& baseFilename,
    size_t maxFileSize,
    bool useCompression)
    : m_outputDir(outputDir)
    , m_baseFilename(baseFilename)
    , m_maxFileSize(maxFileSize)
    , m_useCompression(useCompression)
    , m_enabled(false)
    , m_shutdown(false)
{
    // Create output directory if it doesn't exist
    if (!m_outputDir.empty()) {
        try {
            fs::create_directories(m_outputDir);
        } catch (const std::exception& e) {
            std::cerr << "Error creating output directory: " << e.what() << std::endl;
        }
    }
    
    // Start worker thread
    m_workerThread = std::thread(&AIDatasetLogger::workerThreadFunc, this);
}

AIDatasetLogger::~AIDatasetLogger() {
    // Set shutdown flag
    m_shutdown = true;
    
    // Signal worker thread and wait for it to finish
    m_queueCV.notify_one();
    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
    
    // Close output file if open
    if (m_outputFile.is_open()) {
        m_outputFile.close();
    }
}

bool AIDatasetLogger::logFrame(
    const GameObservation& observation,
    const InputAction& action,
    uint64_t frameNumber,
    float reward)
{
    if (!m_enabled) {
        return false;
    }
    
    // Format the log entry
    std::string logEntry = formatLogEntry(observation, action, frameNumber, reward);
    
    // Queue the log entry
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_logQueue.push(logEntry);
    }
    
    // Signal worker thread
    m_queueCV.notify_one();
    
    return true;
}

void AIDatasetLogger::flush() {
    // Signal worker thread to flush
    m_queueCV.notify_one();
    
    // Wait until queue is empty
    std::unique_lock<std::mutex> lock(m_queueMutex);
    bool queueEmpty = m_queueCV.wait_for(lock, std::chrono::seconds(5), 
        [this]() { return m_logQueue.empty(); });
    
    if (!queueEmpty) {
        std::cerr << "Warning: Flush timed out, queue still has entries" << std::endl;
    }
    
    // Flush file
    if (m_outputFile.is_open()) {
        m_outputFile.flush();
    }
}

bool AIDatasetLogger::startNewLogFile() {
    // Generate a new filename with timestamp
    m_currentFilename = m_outputDir + "/" + m_baseFilename + "_" + generateTimestamp() + ".jsonl";
    
    // Close existing file if open
    if (m_outputFile.is_open()) {
        m_outputFile.close();
    }
    
    // Open new file
    m_outputFile.open(m_currentFilename, std::ios::out | std::ios::app);
    
    return m_outputFile.is_open();
}

void AIDatasetLogger::setEnabled(bool enabled) {
    if (enabled && !m_enabled) {
        // Start a new log file when enabling
        if (!startNewLogFile()) {
            std::cerr << "Error: Failed to start log file" << std::endl;
            return;
        }
    }
    
    m_enabled = enabled;
}

bool AIDatasetLogger::isEnabled() const {
    return m_enabled;
}

void AIDatasetLogger::workerThreadFunc() {
    while (!m_shutdown) {
        std::queue<std::string> localQueue;
        
        // Wait for data or shutdown
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_queueCV.wait(lock, [this]() {
                return !m_logQueue.empty() || m_shutdown;
            });
            
            // Swap queues to minimize lock time
            if (!m_logQueue.empty()) {
                localQueue.swap(m_logQueue);
            }
        }
        
        // Process local queue
        while (!localQueue.empty()) {
            if (m_outputFile.is_open()) {
                m_outputFile << localQueue.front() << std::endl;
                
                // Check if we need to rotate the file
                if (m_outputFile.tellp() >= static_cast<std::streampos>(m_maxFileSize)) {
                    rotateLogFile();
                }
            }
            
            localQueue.pop();
        }
        
        // Flush file periodically
        if (m_outputFile.is_open()) {
            m_outputFile.flush();
        }
    }
}

std::string AIDatasetLogger::generateTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto now_time = std::chrono::system_clock::to_time_t(now);
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_time), "%Y%m%d_%H%M%S");
    ss << "_" << std::setfill('0') << std::setw(3) << now_ms.count();
    
    return ss.str();
}

std::string AIDatasetLogger::formatLogEntry(
    const GameObservation& observation,
    const InputAction& action,
    uint64_t frameNumber,
    float reward)
{
    // Create JSON object
    json entry;
    
    // Add frame number and timestamp
    entry["frame"] = frameNumber;
    entry["timestamp"] = generateTimestamp();
    
    // Add reward if non-zero
    if (reward != 0.0f) {
        entry["reward"] = reward;
    }
    
    // Add game variables
    if (observation.numVariables > 0 && observation.gameVariables != nullptr) {
        json variables = json::array();
        for (int i = 0; i < observation.numVariables; ++i) {
            variables.push_back(observation.gameVariables[i]);
        }
        entry["variables"] = variables;
    }
    
    // Add screen dimensions
    entry["screen_width"] = observation.width;
    entry["screen_height"] = observation.height;
    
    // Add action
    json actionObj;
    actionObj["up"] = action.up;
    actionObj["down"] = action.down;
    actionObj["left"] = action.left;
    actionObj["right"] = action.right;
    actionObj["button1"] = action.button1;
    actionObj["button2"] = action.button2;
    actionObj["button3"] = action.button3;
    actionObj["button4"] = action.button4;
    actionObj["button5"] = action.button5;
    actionObj["button6"] = action.button6;
    actionObj["start"] = action.start;
    actionObj["coin"] = action.coin;
    entry["action"] = actionObj;
    
    // Optionally, we could include a base64 encoding of the screen buffer here
    // But that would make the log file very large
    // Consider only including if explicitly requested or for debugging
    
    // Convert to string and return
    return entry.dump();
}

bool AIDatasetLogger::rotateLogFile() {
    // Close current file
    if (m_outputFile.is_open()) {
        m_outputFile.close();
    }
    
    // Compress the file if enabled
    if (m_useCompression) {
        compressFile(m_currentFilename);
    }
    
    // Start a new file
    return startNewLogFile();
}

bool AIDatasetLogger::compressFile(const std::string& filename) {
    // Ensure file exists
    if (!fs::exists(filename)) {
        return false;
    }
    
    std::string compressCmd;
    std::string outputFile = filename + ".gz";
    
#ifdef _WIN32
    // For Windows, we'd need to have gzip in PATH or specify full path
    compressCmd = "gzip -c \"" + filename + "\" > \"" + outputFile + "\"";
    // Use CreateProcess for Windows if needed
    
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    
    if (!CreateProcess(NULL, const_cast<LPSTR>(compressCmd.c_str()), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        std::cerr << "CreateProcess failed (" << GetLastError() << ")" << std::endl;
        return false;
    }
    
    // Wait for the process to finish
    WaitForSingleObject(pi.hProcess, INFINITE);
    
    // Close process and thread handles
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
#else
    // For Unix-like systems
    compressCmd = "gzip -c '" + filename + "' > '" + outputFile + "'";
    int result = system(compressCmd.c_str());
    if (result != 0) {
        std::cerr << "Error compressing file: " << result << std::endl;
        return false;
    }
#endif
    
    // Delete the original file if compression succeeded
    if (fs::exists(outputFile)) {
        fs::remove(filename);
        return true;
    }
    
    return false;
}

} // namespace AI 