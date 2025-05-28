#pragma once

#include "disassembly_viewer.h"
#include "breakpoint_manager.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <functional>
#include <vector>

// Forward declarations
class MemoryViewer;
class RegisterViewer;
class CallStackViewer;
class WatchViewer;
class WatchpointManager;

/**
 * @class DebugVisualizer
 * @brief Manages and coordinates all debug visualization components
 * 
 * This class acts as the main interface for the debugging UI, managing
 * and coordinating the disassembly viewer, memory viewer, breakpoint manager,
 * and other debugging components. It provides a unified interface for the
 * debugger UI.
 */
class DebugVisualizer {
public:
    /**
     * @enum DebuggerState
     * @brief Represents the current state of the debugger
     */
    enum class DebuggerState {
        Inactive,   ///< Debugger is not active
        Running,    ///< Game is running normally
        Paused,     ///< Game is paused due to breakpoint or user request
        StepOver,   ///< Stepping over an instruction
        StepInto,   ///< Stepping into a call instruction
        StepOut     ///< Stepping out of the current function
    };

public:
    /**
     * @brief Constructor
     */
    DebugVisualizer();
    
    /**
     * @brief Destructor
     */
    ~DebugVisualizer();
    
    /**
     * @brief Initialize the debug visualizer
     * @param cpuArchitecture The primary CPU architecture (e.g., "M68K", "Z80")
     * @return True if successfully initialized, false otherwise
     */
    bool initialize(const std::string& cpuArchitecture);
    
    /**
     * @brief Shutdown the debug visualizer
     */
    void shutdown();
    
    /**
     * @brief Update the debug visualizer state
     * @param deltaTime Time elapsed since last update
     */
    void update(float deltaTime);
    
    /**
     * @brief Render the debug visualizer
     * @param x X coordinate for rendering
     * @param y Y coordinate for rendering
     * @param width Width of the rendering area
     * @param height Height of the rendering area
     */
    void render(float x, float y, float width, float height);
    
    /**
     * @brief Get the current debugger state
     * @return The current state
     */
    DebuggerState getState() const;
    
    /**
     * @brief Set the debugger state
     * @param state The new state
     */
    void setState(DebuggerState state);
    
    /**
     * @brief Get the disassembly viewer
     * @return The disassembly viewer for the active architecture
     */
    std::shared_ptr<DisassemblyViewer> getDisassemblyViewer();
    
    /**
     * @brief Get the memory viewer
     * @return The memory viewer
     */
    std::shared_ptr<MemoryViewer> getMemoryViewer();
    
    /**
     * @brief Get the register viewer
     * @return The register viewer
     */
    std::shared_ptr<RegisterViewer> getRegisterViewer();
    
    /**
     * @brief Get the call stack viewer
     * @return The call stack viewer
     */
    std::shared_ptr<CallStackViewer> getCallStackViewer();
    
    /**
     * @brief Get the watch viewer
     * @return The watch viewer
     */
    std::shared_ptr<WatchViewer> getWatchViewer();
    
    /**
     * @brief Get the breakpoint manager
     * @return The breakpoint manager
     */
    std::shared_ptr<BreakpointManager> getBreakpointManager();
    
    /**
     * @brief Set the current execution address (PC)
     * @param address The execution address
     * @param cpuArchitecture The CPU architecture
     */
    void setExecutionAddress(uint32_t address, const std::string& cpuArchitecture);
    
    /**
     * @brief Set memory read function for a CPU architecture
     * @param cpuArchitecture The CPU architecture
     * @param readFunction Function that reads a byte from memory
     */
    void setMemoryReadFunction(const std::string& cpuArchitecture,
                              std::function<uint8_t(uint32_t)> readFunction);
    
    /**
     * @brief Set memory write function for a CPU architecture
     * @param cpuArchitecture The CPU architecture
     * @param writeFunction Function that writes a byte to memory
     */
    void setMemoryWriteFunction(const std::string& cpuArchitecture,
                               std::function<void(uint32_t, uint8_t)> writeFunction);
    
    /**
     * @brief Add a CPU architecture to the debugger
     * @param cpuArchitecture The CPU architecture (e.g., "M68K", "Z80")
     * @return True if the architecture was added successfully
     */
    bool addCpuArchitecture(const std::string& cpuArchitecture);
    
    /**
     * @brief Set the active CPU architecture
     * @param cpuArchitecture The CPU architecture
     * @return True if the architecture was set successfully
     */
    bool setActiveCpuArchitecture(const std::string& cpuArchitecture);
    
    /**
     * @brief Get the active CPU architecture
     * @return The active CPU architecture
     */
    std::string getActiveCpuArchitecture() const;
    
    /**
     * @brief Check if a breakpoint is hit at the current execution address
     * @return True if a breakpoint is hit, false otherwise
     */
    bool checkBreakpoints();
    
    /**
     * @brief Handle stepping (over, into, out)
     * @return True if stepping is complete, false if still stepping
     */
    bool handleStepping();
    
    /**
     * @brief Pause the emulation
     */
    void pauseEmulation();
    
    /**
     * @brief Resume the emulation
     */
    void resumeEmulation();
    
    /**
     * @brief Step over the current instruction
     */
    void stepOver();
    
    /**
     * @brief Step into the current instruction (if it's a call)
     */
    void stepInto();
    
    /**
     * @brief Step out of the current function
     */
    void stepOut();
    
    /**
     * @brief Toggle a breakpoint at the specified address
     * @param address The address to toggle the breakpoint at
     * @param cpuArchitecture The CPU architecture
     * @return True if a breakpoint was added, false if removed
     */
    bool toggleBreakpoint(uint32_t address, const std::string& cpuArchitecture);

    /**
     * @brief Set register values for a CPU architecture
     * @param cpuArchitecture The CPU architecture
     * @param registers Map of register names to values
     */
    void setRegisterValues(const std::string& cpuArchitecture,
                          const std::unordered_map<std::string, uint64_t>& registers);

private:
    DebuggerState m_state;
    std::string m_activeCpuArchitecture;
    std::unordered_map<std::string, std::shared_ptr<DisassemblyViewer>> m_disassemblyViewers;
    std::shared_ptr<MemoryViewer> m_memoryViewer;
    std::shared_ptr<RegisterViewer> m_registerViewer;
    std::shared_ptr<CallStackViewer> m_callStackViewer;
    std::shared_ptr<WatchViewer> m_watchViewer;
    std::shared_ptr<BreakpointManager> m_breakpointManager;
    
    // Current execution state for step over/out
    uint32_t m_stepOverReturnAddress;
    uint32_t m_stepOutStackLevel;
    
    // Memory access functions by architecture
    std::unordered_map<std::string, std::function<uint8_t(uint32_t)>> m_memoryReadFunctions;
    std::unordered_map<std::string, std::function<void(uint32_t, uint8_t)>> m_memoryWriteFunctions;
}; 