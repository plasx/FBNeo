#include "breakpoint_manager.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>

// Constructor
BreakpointManager::BreakpointManager()
    : m_nextBreakpointId(1)
{
    // Default condition evaluator just returns true
    m_conditionEvaluator = [](const std::string&) { return true; };
}

// Destructor
BreakpointManager::~BreakpointManager()
{
}

// Generate a unique breakpoint ID
uint32_t BreakpointManager::generateBreakpointId()
{
    return m_nextBreakpointId++;
}

// Create a unique key for address+architecture combination
uint64_t BreakpointManager::getBreakpointKey(uint32_t address, const std::string& architecture) const
{
    // Combine address and hash of architecture into a 64-bit key
    uint64_t key = address;
    size_t archHash = std::hash<std::string>{}(architecture);
    key |= (static_cast<uint64_t>(archHash) << 32);
    return key;
}

// Add a new breakpoint
uint32_t BreakpointManager::addBreakpoint(
    uint32_t address, 
    const std::string& architecture, 
    const std::string& condition, 
    const std::string& description)
{
    // Check if a breakpoint already exists at this address/architecture
    uint64_t key = getBreakpointKey(address, architecture);
    auto existingIt = m_addressToIdMap.find(key);
    if (existingIt != m_addressToIdMap.end()) {
        // Return the existing breakpoint ID
        return existingIt->second;
    }
    
    // Create a new breakpoint
    uint32_t id = generateBreakpointId();
    Breakpoint bp;
    bp.address = address;
    bp.architecture = architecture;
    bp.condition = condition;
    bp.description = description;
    bp.enabled = true;
    bp.hitCount = 0;
    bp.ignoreCount = 0;
    
    // Add it to our collections
    m_breakpoints[id] = bp;
    m_addressToIdMap[key] = id;
    
    // Notify of change
    if (m_breakpointsChangedCallback) {
        m_breakpointsChangedCallback();
    }
    
    return id;
}

// Remove a breakpoint by ID
bool BreakpointManager::removeBreakpoint(uint32_t id)
{
    auto it = m_breakpoints.find(id);
    if (it == m_breakpoints.end()) {
        return false;
    }
    
    // Remove from address map
    uint64_t key = getBreakpointKey(it->second.address, it->second.architecture);
    m_addressToIdMap.erase(key);
    
    // Remove from breakpoints map
    m_breakpoints.erase(it);
    
    // Notify of change
    if (m_breakpointsChangedCallback) {
        m_breakpointsChangedCallback();
    }
    
    return true;
}

// Enable or disable a breakpoint
bool BreakpointManager::enableBreakpoint(uint32_t id, bool enabled)
{
    auto it = m_breakpoints.find(id);
    if (it == m_breakpoints.end()) {
        return false;
    }
    
    it->second.enabled = enabled;
    
    // Notify of change
    if (m_breakpointsChangedCallback) {
        m_breakpointsChangedCallback();
    }
    
    return true;
}

// Check if a breakpoint exists at the given address
bool BreakpointManager::hasBreakpoint(uint32_t address, const std::string& architecture) const
{
    uint64_t key = getBreakpointKey(address, architecture);
    return m_addressToIdMap.find(key) != m_addressToIdMap.end();
}

// Get a breakpoint by ID
const BreakpointManager::Breakpoint* BreakpointManager::getBreakpoint(uint32_t id) const
{
    auto it = m_breakpoints.find(id);
    if (it == m_breakpoints.end()) {
        return nullptr;
    }
    return &it->second;
}

// Get all breakpoints for a specific architecture
std::vector<uint32_t> BreakpointManager::getBreakpointsForArchitecture(const std::string& architecture) const
{
    std::vector<uint32_t> result;
    
    for (const auto& pair : m_breakpoints) {
        if (pair.second.architecture == architecture) {
            result.push_back(pair.first);
        }
    }
    
    return result;
}

// Get all breakpoints
const std::unordered_map<uint32_t, BreakpointManager::Breakpoint>& BreakpointManager::getAllBreakpoints() const
{
    return m_breakpoints;
}

// Check if execution should break at the current address
bool BreakpointManager::shouldBreak(uint32_t address, const std::string& architecture)
{
    uint64_t key = getBreakpointKey(address, architecture);
    auto it = m_addressToIdMap.find(key);
    if (it == m_addressToIdMap.end()) {
        return false;
    }
    
    uint32_t bpId = it->second;
    auto& bp = m_breakpoints[bpId];
    
    // If the breakpoint is disabled, don't break
    if (!bp.enabled) {
        return false;
    }
    
    // Increment hit count
    bp.hitCount++;
    
    // If we haven't reached the ignore count, don't break
    if (bp.hitCount <= bp.ignoreCount) {
        return false;
    }
    
    // Evaluate condition if present
    if (!bp.condition.empty() && m_conditionEvaluator) {
        return m_conditionEvaluator(bp.condition);
    }
    
    // No condition or no evaluator, just break
    return true;
}

// Set condition evaluator function
void BreakpointManager::setConditionEvaluator(std::function<bool(const std::string&)> evaluator)
{
    m_conditionEvaluator = evaluator;
}

// Set callback for when breakpoints are changed
void BreakpointManager::setBreakpointsChangedCallback(std::function<void()> callback)
{
    m_breakpointsChangedCallback = callback;
}

// Save breakpoints to a file
bool BreakpointManager::saveBreakpoints(const std::string& filename) const
{
    try {
        std::ofstream file(filename);
        if (!file.is_open()) {
            return false;
        }
        
        // File format:
        // <address>,<architecture>,<enabled>,<condition>,<description>,<ignoreCount>
        
        for (const auto& pair : m_breakpoints) {
            const auto& bp = pair.second;
            
            file << std::hex << bp.address << ","
                 << bp.architecture << ","
                 << (bp.enabled ? "1" : "0") << ","
                 << bp.condition << ","
                 << bp.description << ","
                 << std::dec << bp.ignoreCount << std::endl;
        }
        
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error saving breakpoints: " << e.what() << std::endl;
        return false;
    }
}

// Load breakpoints from a file
bool BreakpointManager::loadBreakpoints(const std::string& filename)
{
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            return false;
        }
        
        // Clear existing breakpoints
        clearAllBreakpoints();
        
        std::string line;
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string token;
            
            std::vector<std::string> tokens;
            while (std::getline(iss, token, ',')) {
                tokens.push_back(token);
            }
            
            if (tokens.size() < 6) {
                continue;
            }
            
            uint32_t address;
            std::istringstream(tokens[0]) >> std::hex >> address;
            
            std::string architecture = tokens[1];
            bool enabled = (tokens[2] == "1");
            std::string condition = tokens[3];
            std::string description = tokens[4];
            
            int ignoreCount;
            std::istringstream(tokens[5]) >> std::dec >> ignoreCount;
            
            // Add the breakpoint
            uint32_t id = addBreakpoint(address, architecture, condition, description);
            enableBreakpoint(id, enabled);
            
            // Set ignore count
            if (id != 0 && ignoreCount > 0) {
                auto it = m_breakpoints.find(id);
                if (it != m_breakpoints.end()) {
                    it->second.ignoreCount = ignoreCount;
                }
            }
        }
        
        // Notify of change
        if (m_breakpointsChangedCallback) {
            m_breakpointsChangedCallback();
        }
        
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error loading breakpoints: " << e.what() << std::endl;
        return false;
    }
}

// Clear all breakpoints
void BreakpointManager::clearAllBreakpoints()
{
    m_breakpoints.clear();
    m_addressToIdMap.clear();
    
    // Notify of change
    if (m_breakpointsChangedCallback) {
        m_breakpointsChangedCallback();
    }
} 