#include "watchpoint_manager.h"
#include <iostream>
#include <algorithm>
#include <sstream>
#include <regex>

struct WatchpointManager::Impl {
    // Implementation-specific members and methods
};

WatchpointManager::WatchpointManager() 
    : m_impl(std::make_unique<Impl>())
    , m_nextWatchpointId(1) {
}

WatchpointManager::~WatchpointManager() {
    shutdown();
}

bool WatchpointManager::initialize() {
    m_nextWatchpointId = 1;
    m_watchpoints.clear();
    return true;
}

void WatchpointManager::shutdown() {
    clearAllWatchpoints();
}

uint32_t WatchpointManager::addWatchpoint(uint32_t address, uint32_t size, AccessType accessType,
                                       DataType dataType, const std::string& name,
                                       const std::string& condition, bool enabled,
                                       const std::string& cpuArchitecture) {
    Watchpoint watchpoint;
    watchpoint.id = m_nextWatchpointId++;
    watchpoint.name = name.empty() ? "Watchpoint " + std::to_string(watchpoint.id) : name;
    watchpoint.address = address;
    watchpoint.size = size;
    watchpoint.accessType = accessType;
    watchpoint.dataType = dataType;
    watchpoint.enabled = enabled;
    watchpoint.condition = condition;
    watchpoint.previousValue = 0;
    watchpoint.currentValue = readMemoryValue(address, dataType);
    watchpoint.previousValue = watchpoint.currentValue;
    watchpoint.hasTriggered = false;
    watchpoint.cpuArchitecture = cpuArchitecture;

    m_watchpoints[watchpoint.id] = watchpoint;
    
    if (m_addedCallback) {
        m_addedCallback(watchpoint);
    }
    
    return watchpoint.id;
}

bool WatchpointManager::removeWatchpoint(uint32_t id) {
    auto it = m_watchpoints.find(id);
    if (it == m_watchpoints.end()) {
        return false;
    }
    
    if (m_removedCallback) {
        m_removedCallback(id);
    }
    
    m_watchpoints.erase(it);
    return true;
}

bool WatchpointManager::enableWatchpoint(uint32_t id, bool enable) {
    auto it = m_watchpoints.find(id);
    if (it == m_watchpoints.end()) {
        return false;
    }
    
    it->second.enabled = enable;
    
    if (m_changedCallback) {
        m_changedCallback(it->second);
    }
    
    return true;
}

bool WatchpointManager::setWatchpointCondition(uint32_t id, const std::string& condition) {
    auto it = m_watchpoints.find(id);
    if (it == m_watchpoints.end()) {
        return false;
    }
    
    it->second.condition = condition;
    
    if (m_changedCallback) {
        m_changedCallback(it->second);
    }
    
    return true;
}

bool WatchpointManager::getWatchpoint(uint32_t id, Watchpoint& watchpoint) const {
    auto it = m_watchpoints.find(id);
    if (it == m_watchpoints.end()) {
        return false;
    }
    
    watchpoint = it->second;
    return true;
}

std::vector<WatchpointManager::Watchpoint> WatchpointManager::getAllWatchpoints() const {
    std::vector<Watchpoint> result;
    result.reserve(m_watchpoints.size());
    
    for (const auto& pair : m_watchpoints) {
        result.push_back(pair.second);
    }
    
    return result;
}

void WatchpointManager::clearAllWatchpoints() {
    // Notify about removal of each watchpoint
    if (m_removedCallback) {
        for (const auto& pair : m_watchpoints) {
            m_removedCallback(pair.first);
        }
    }
    
    m_watchpoints.clear();
}

void WatchpointManager::setWatchpointTriggeredCallback(WatchpointCallback callback) {
    m_triggeredCallback = callback;
}

void WatchpointManager::setWatchpointAddedCallback(WatchpointCallback callback) {
    m_addedCallback = callback;
}

void WatchpointManager::setWatchpointRemovedCallback(std::function<void(uint32_t)> callback) {
    m_removedCallback = callback;
}

void WatchpointManager::setWatchpointChangedCallback(WatchpointCallback callback) {
    m_changedCallback = callback;
}

bool WatchpointManager::checkMemoryRead(uint32_t address, uint64_t value, uint32_t size, const std::string& cpuArchitecture) {
    if (!isAddressWatched(address, size, AccessType::Read, cpuArchitecture) &&
        !isAddressWatched(address, size, AccessType::ReadWrite, cpuArchitecture)) {
        return false;
    }
    
    bool triggered = false;
    auto watchpoints = getWatchpointsForAddress(address, size, AccessType::Read, cpuArchitecture);
    auto readWriteWatchpoints = getWatchpointsForAddress(address, size, AccessType::ReadWrite, cpuArchitecture);
    watchpoints.insert(watchpoints.end(), readWriteWatchpoints.begin(), readWriteWatchpoints.end());
    
    for (auto* watchpoint : watchpoints) {
        if (!watchpoint->enabled) {
            continue;
        }
        
        watchpoint->previousValue = watchpoint->currentValue;
        watchpoint->currentValue = value;
        
        // Check condition if specified
        bool conditionMet = true;
        if (!watchpoint->condition.empty()) {
            if (!evaluateCondition(watchpoint->condition, value)) {
                continue;
            }
        }
        
        watchpoint->hasTriggered = true;
        triggered = true;
        
        if (m_triggeredCallback) {
            m_triggeredCallback(*watchpoint);
        }
    }
    
    return triggered;
}

bool WatchpointManager::checkMemoryWrite(uint32_t address, uint64_t value, uint32_t size, const std::string& cpuArchitecture) {
    if (!isAddressWatched(address, size, AccessType::Write, cpuArchitecture) &&
        !isAddressWatched(address, size, AccessType::ReadWrite, cpuArchitecture)) {
        return false;
    }
    
    bool triggered = false;
    auto watchpoints = getWatchpointsForAddress(address, size, AccessType::Write, cpuArchitecture);
    auto readWriteWatchpoints = getWatchpointsForAddress(address, size, AccessType::ReadWrite, cpuArchitecture);
    watchpoints.insert(watchpoints.end(), readWriteWatchpoints.begin(), readWriteWatchpoints.end());
    
    for (auto* watchpoint : watchpoints) {
        if (!watchpoint->enabled) {
            continue;
        }
        
        watchpoint->previousValue = watchpoint->currentValue;
        watchpoint->currentValue = value;
        
        // Check condition if specified
        if (!watchpoint->condition.empty()) {
            if (!evaluateCondition(watchpoint->condition, value)) {
                continue;
            }
        }
        
        watchpoint->hasTriggered = true;
        triggered = true;
        
        if (m_triggeredCallback) {
            m_triggeredCallback(*watchpoint);
        }
    }
    
    return triggered;
}

void WatchpointManager::update() {
    for (auto& pair : m_watchpoints) {
        Watchpoint& watchpoint = pair.second;
        if (!watchpoint.enabled) {
            continue;
        }
        
        uint64_t currentValue = readMemoryValue(watchpoint.address, watchpoint.dataType);
        
        if (currentValue != watchpoint.currentValue) {
            watchpoint.previousValue = watchpoint.currentValue;
            watchpoint.currentValue = currentValue;
            
            if (watchpoint.condition.empty() || evaluateCondition(watchpoint.condition, currentValue)) {
                watchpoint.hasTriggered = true;
                
                if (m_triggeredCallback) {
                    m_triggeredCallback(watchpoint);
                }
            }
        }
    }
}

uint64_t WatchpointManager::getWatchpointValue(uint32_t id) const {
    auto it = m_watchpoints.find(id);
    if (it == m_watchpoints.end()) {
        return 0;
    }
    
    return it->second.currentValue;
}

bool WatchpointManager::hasWatchpointValueChanged(uint32_t id) const {
    auto it = m_watchpoints.find(id);
    if (it == m_watchpoints.end()) {
        return false;
    }
    
    return it->second.previousValue != it->second.currentValue;
}

bool WatchpointManager::evaluateCondition(const std::string& condition, uint64_t value) const {
    bool result = false;
    if (parseExpression(condition, value, result)) {
        return result;
    }
    
    // Default to true if we can't parse the expression
    return true;
}

bool WatchpointManager::isAddressWatched(uint32_t address, uint32_t accessSize, AccessType accessType, const std::string& cpuArchitecture) const {
    for (const auto& pair : m_watchpoints) {
        const Watchpoint& watchpoint = pair.second;
        
        if (!watchpoint.enabled) {
            continue;
        }
        
        if (watchpoint.accessType != accessType && watchpoint.accessType != AccessType::ReadWrite) {
            continue;
        }
        
        if (!cpuArchitecture.empty() && !watchpoint.cpuArchitecture.empty() && 
            cpuArchitecture != watchpoint.cpuArchitecture) {
            continue;
        }
        
        uint32_t watchpointEnd = watchpoint.address + watchpoint.size - 1;
        uint32_t accessEnd = address + accessSize - 1;
        
        // Check if there's an overlap
        if (!(watchpointEnd < address || watchpoint.address > accessEnd)) {
            return true;
        }
    }
    
    return false;
}

std::vector<WatchpointManager::Watchpoint*> WatchpointManager::getWatchpointsForAddress(uint32_t address, uint32_t accessSize, AccessType accessType, const std::string& cpuArchitecture) {
    std::vector<Watchpoint*> result;
    
    for (auto& pair : m_watchpoints) {
        Watchpoint& watchpoint = pair.second;
        
        if (!watchpoint.enabled) {
            continue;
        }
        
        if (watchpoint.accessType != accessType && watchpoint.accessType != AccessType::ReadWrite) {
            continue;
        }
        
        if (!cpuArchitecture.empty() && !watchpoint.cpuArchitecture.empty() && 
            cpuArchitecture != watchpoint.cpuArchitecture) {
            continue;
        }
        
        uint32_t watchpointEnd = watchpoint.address + watchpoint.size - 1;
        uint32_t accessEnd = address + accessSize - 1;
        
        // Check if there's an overlap
        if (!(watchpointEnd < address || watchpoint.address > accessEnd)) {
            result.push_back(&watchpoint);
        }
    }
    
    return result;
}

uint64_t WatchpointManager::readMemoryValue(uint32_t address, DataType dataType) const {
    // This function would typically call into the emulator's memory system
    // For now, return a placeholder value
    return 0;
}

bool WatchpointManager::parseExpression(const std::string& expression, uint64_t currentValue, bool& result) const {
    // This is a simplified expression parser
    // In a real implementation, you would want a more robust expression evaluator
    
    // Replace $value with the actual value
    std::string expr = expression;
    std::regex valueRegex("\\$value");
    expr = std::regex_replace(expr, valueRegex, std::to_string(currentValue));
    
    // Very simple expression parser for basic conditions
    try {
        // Check for simple comparisons
        if (expr.find("==") != std::string::npos) {
            size_t pos = expr.find("==");
            std::string leftStr = expr.substr(0, pos);
            std::string rightStr = expr.substr(pos + 2);
            
            // Trim whitespace
            leftStr.erase(0, leftStr.find_first_not_of(" \t"));
            leftStr.erase(leftStr.find_last_not_of(" \t") + 1);
            rightStr.erase(0, rightStr.find_first_not_of(" \t"));
            rightStr.erase(rightStr.find_last_not_of(" \t") + 1);
            
            uint64_t left = std::stoull(leftStr);
            uint64_t right = std::stoull(rightStr);
            
            result = (left == right);
            return true;
        }
        else if (expr.find("!=") != std::string::npos) {
            size_t pos = expr.find("!=");
            std::string leftStr = expr.substr(0, pos);
            std::string rightStr = expr.substr(pos + 2);
            
            // Trim whitespace
            leftStr.erase(0, leftStr.find_first_not_of(" \t"));
            leftStr.erase(leftStr.find_last_not_of(" \t") + 1);
            rightStr.erase(0, rightStr.find_first_not_of(" \t"));
            rightStr.erase(rightStr.find_last_not_of(" \t") + 1);
            
            uint64_t left = std::stoull(leftStr);
            uint64_t right = std::stoull(rightStr);
            
            result = (left != right);
            return true;
        }
        else if (expr.find("<") != std::string::npos) {
            size_t pos = expr.find("<");
            if (pos > 0 && expr[pos-1] == '=') {
                pos--; // Handle <= operator
                std::string leftStr = expr.substr(0, pos);
                std::string rightStr = expr.substr(pos + 2);
                
                // Trim whitespace
                leftStr.erase(0, leftStr.find_first_not_of(" \t"));
                leftStr.erase(leftStr.find_last_not_of(" \t") + 1);
                rightStr.erase(0, rightStr.find_first_not_of(" \t"));
                rightStr.erase(rightStr.find_last_not_of(" \t") + 1);
                
                uint64_t left = std::stoull(leftStr);
                uint64_t right = std::stoull(rightStr);
                
                result = (left <= right);
                return true;
            }
            else {
                std::string leftStr = expr.substr(0, pos);
                std::string rightStr = expr.substr(pos + 1);
                
                // Trim whitespace
                leftStr.erase(0, leftStr.find_first_not_of(" \t"));
                leftStr.erase(leftStr.find_last_not_of(" \t") + 1);
                rightStr.erase(0, rightStr.find_first_not_of(" \t"));
                rightStr.erase(rightStr.find_last_not_of(" \t") + 1);
                
                uint64_t left = std::stoull(leftStr);
                uint64_t right = std::stoull(rightStr);
                
                result = (left < right);
                return true;
            }
        }
        else if (expr.find(">") != std::string::npos) {
            size_t pos = expr.find(">");
            if (pos > 0 && expr[pos-1] == '=') {
                pos--; // Handle >= operator
                std::string leftStr = expr.substr(0, pos);
                std::string rightStr = expr.substr(pos + 2);
                
                // Trim whitespace
                leftStr.erase(0, leftStr.find_first_not_of(" \t"));
                leftStr.erase(leftStr.find_last_not_of(" \t") + 1);
                rightStr.erase(0, rightStr.find_first_not_of(" \t"));
                rightStr.erase(rightStr.find_last_not_of(" \t") + 1);
                
                uint64_t left = std::stoull(leftStr);
                uint64_t right = std::stoull(rightStr);
                
                result = (left >= right);
                return true;
            }
            else {
                std::string leftStr = expr.substr(0, pos);
                std::string rightStr = expr.substr(pos + 1);
                
                // Trim whitespace
                leftStr.erase(0, leftStr.find_first_not_of(" \t"));
                leftStr.erase(leftStr.find_last_not_of(" \t") + 1);
                rightStr.erase(0, rightStr.find_first_not_of(" \t"));
                rightStr.erase(rightStr.find_last_not_of(" \t") + 1);
                
                uint64_t left = std::stoull(leftStr);
                uint64_t right = std::stoull(rightStr);
                
                result = (left > right);
                return true;
            }
        }
        
        // If no comparison operator is found, convert the expression to a boolean
        result = std::stoull(expr) != 0;
        return true;
    }
    catch (const std::exception&) {
        return false;
    }
}

uint32_t WatchpointManager::getDataTypeSize(DataType dataType) const {
    switch (dataType) {
        case DataType::Byte:
            return 1;
        case DataType::Word:
            return 2;
        case DataType::DWord:
            return 4;
        case DataType::QWord:
            return 8;
        default:
            return 1;
    }
} 