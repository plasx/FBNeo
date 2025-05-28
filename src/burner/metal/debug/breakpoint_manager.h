#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include <memory>

/**
 * @class BreakpointManager
 * @brief Manages breakpoints for the debugger system
 * 
 * This class provides functionality for adding, removing, and managing breakpoints
 * across multiple CPU architectures. It supports conditional breakpoints,
 * hit counting, and can be integrated with the CPU emulation core to handle
 * breakpoint triggers during emulation.
 */
class BreakpointManager {
public:
    /**
     * @struct Breakpoint
     * @brief Structure representing a debugger breakpoint
     */
    struct Breakpoint {
        uint32_t address;           ///< Memory address where the breakpoint is set
        std::string condition;      ///< Expression that must evaluate to true for breakpoint to trigger
        std::string description;    ///< User description for the breakpoint
        bool enabled;               ///< Whether the breakpoint is currently active
        int hitCount;               ///< Number of times this breakpoint has been hit
        int ignoreCount;            ///< Number of hits to ignore before stopping
        std::string architecture;   ///< CPU architecture this breakpoint is for (e.g., "M68K", "Z80")
    };

public:
    /**
     * @brief Constructor
     */
    BreakpointManager();
    
    /**
     * @brief Destructor
     */
    ~BreakpointManager();
    
    /**
     * @brief Add a new breakpoint
     * @param address The memory address for the breakpoint
     * @param architecture The CPU architecture (e.g., "M68K", "Z80")
     * @param condition Optional condition expression
     * @param description Optional user description
     * @return The ID of the newly created breakpoint
     */
    uint32_t addBreakpoint(uint32_t address, const std::string& architecture, 
                         const std::string& condition = "", 
                         const std::string& description = "");
    
    /**
     * @brief Remove a breakpoint
     * @param id The ID of the breakpoint to remove
     * @return True if breakpoint was found and removed, false otherwise
     */
    bool removeBreakpoint(uint32_t id);
    
    /**
     * @brief Enable or disable a breakpoint
     * @param id The ID of the breakpoint
     * @param enabled True to enable, false to disable
     * @return True if breakpoint was found and updated, false otherwise
     */
    bool enableBreakpoint(uint32_t id, bool enabled);
    
    /**
     * @brief Check if an address has a breakpoint
     * @param address The address to check
     * @param architecture The CPU architecture
     * @return True if a breakpoint exists at the address, false otherwise
     */
    bool hasBreakpoint(uint32_t address, const std::string& architecture) const;
    
    /**
     * @brief Get breakpoint by ID
     * @param id The breakpoint ID
     * @return Pointer to the breakpoint, or nullptr if not found
     */
    const Breakpoint* getBreakpoint(uint32_t id) const;
    
    /**
     * @brief Get all breakpoints for a specific architecture
     * @param architecture The CPU architecture
     * @return Vector of breakpoint IDs for the specified architecture
     */
    std::vector<uint32_t> getBreakpointsForArchitecture(const std::string& architecture) const;
    
    /**
     * @brief Get all breakpoints
     * @return Map of breakpoint IDs to Breakpoint structs
     */
    const std::unordered_map<uint32_t, Breakpoint>& getAllBreakpoints() const;
    
    /**
     * @brief Process execution at an address
     * @param address The current execution address
     * @param architecture The CPU architecture
     * @return True if execution should halt due to a breakpoint, false otherwise
     */
    bool shouldBreak(uint32_t address, const std::string& architecture);
    
    /**
     * @brief Set condition evaluation function
     * @param evaluator Function that takes a condition string and returns whether it's true
     */
    void setConditionEvaluator(std::function<bool(const std::string&)> evaluator);
    
    /**
     * @brief Set breakpoints changed callback
     * @param callback Function to call when breakpoints are added/removed/modified
     */
    void setBreakpointsChangedCallback(std::function<void()> callback);
    
    /**
     * @brief Save breakpoints to a file
     * @param filename The file to save to
     * @return True if successfully saved, false otherwise
     */
    bool saveBreakpoints(const std::string& filename) const;
    
    /**
     * @brief Load breakpoints from a file
     * @param filename The file to load from
     * @return True if successfully loaded, false otherwise
     */
    bool loadBreakpoints(const std::string& filename);
    
    /**
     * @brief Clear all breakpoints
     */
    void clearAllBreakpoints();

private:
    std::unordered_map<uint32_t, Breakpoint> m_breakpoints;
    uint32_t m_nextBreakpointId;
    std::function<bool(const std::string&)> m_conditionEvaluator;
    std::function<void()> m_breakpointsChangedCallback;
    
    uint32_t generateBreakpointId();
    uint64_t getBreakpointKey(uint32_t address, const std::string& architecture) const;
    std::unordered_map<uint64_t, uint32_t> m_addressToIdMap;
}; 