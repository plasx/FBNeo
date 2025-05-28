#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>

class WatchpointManager {
public:
    enum class AccessType {
        Read,
        Write,
        ReadWrite
    };

    enum class DataType {
        Byte,       // 8-bit
        Word,       // 16-bit
        DWord,      // 32-bit
        QWord       // 64-bit
    };

    struct Watchpoint {
        uint32_t id;
        std::string name;
        uint32_t address;
        uint32_t size;
        AccessType accessType;
        DataType dataType;
        bool enabled;
        std::string condition;
        uint64_t previousValue;
        uint64_t currentValue;
        bool hasTriggered;
        std::string cpuArchitecture;
    };

    typedef std::function<void(const Watchpoint&)> WatchpointCallback;

    WatchpointManager();
    ~WatchpointManager();

    bool initialize();
    void shutdown();

    // Watchpoint management
    uint32_t addWatchpoint(uint32_t address, uint32_t size, AccessType accessType,
                         DataType dataType, const std::string& name = "",
                         const std::string& condition = "", bool enabled = true,
                         const std::string& cpuArchitecture = "");
    bool removeWatchpoint(uint32_t id);
    bool enableWatchpoint(uint32_t id, bool enable);
    bool setWatchpointCondition(uint32_t id, const std::string& condition);
    bool getWatchpoint(uint32_t id, Watchpoint& watchpoint) const;
    std::vector<Watchpoint> getAllWatchpoints() const;
    void clearAllWatchpoints();

    // Notification callbacks
    void setWatchpointTriggeredCallback(WatchpointCallback callback);
    void setWatchpointAddedCallback(WatchpointCallback callback);
    void setWatchpointRemovedCallback(std::function<void(uint32_t)> callback);
    void setWatchpointChangedCallback(WatchpointCallback callback);

    // Memory access hooks
    bool checkMemoryRead(uint32_t address, uint64_t value, uint32_t size, const std::string& cpuArchitecture = "");
    bool checkMemoryWrite(uint32_t address, uint64_t value, uint32_t size, const std::string& cpuArchitecture = "");

    // Update watchpoints state
    void update();

    // Watchpoint value querying
    uint64_t getWatchpointValue(uint32_t id) const;
    bool hasWatchpointValueChanged(uint32_t id) const;

    // Condition evaluation
    bool evaluateCondition(const std::string& condition, uint64_t value) const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;

    uint32_t m_nextWatchpointId;
    WatchpointCallback m_triggeredCallback;
    WatchpointCallback m_addedCallback;
    std::function<void(uint32_t)> m_removedCallback;
    WatchpointCallback m_changedCallback;
    std::unordered_map<uint32_t, Watchpoint> m_watchpoints;

    bool isAddressWatched(uint32_t address, uint32_t accessSize, AccessType accessType, const std::string& cpuArchitecture) const;
    std::vector<Watchpoint*> getWatchpointsForAddress(uint32_t address, uint32_t accessSize, AccessType accessType, const std::string& cpuArchitecture);
    uint64_t readMemoryValue(uint32_t address, DataType dataType) const;
    bool parseExpression(const std::string& expression, uint64_t currentValue, bool& result) const;
    uint32_t getDataTypeSize(DataType dataType) const;
}; 