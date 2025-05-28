#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

#include "memory_viewer.h"

// Forward declarations
class MetalContext;
class Disassembler;
class BreakpointManager;
class WatchpointManager;
class RegisterViewer;

/**
 * @brief Main debugger class that integrates all debugging components
 * 
 * The Debugger provides an interface for debugging and inspecting the
 * emulated system's state, including memory inspection, disassembly,
 * breakpoints, and CPU register monitoring.
 */
class Debugger {
public:
    Debugger();
    ~Debugger();

    /**
     * @brief Initialize the debugger with the Metal rendering context
     * 
     * @param metalContext Pointer to the Metal rendering context
     * @return bool True if initialization succeeded
     */
    bool initialize(MetalContext* metalContext);

    /**
     * @brief Update the debugger state
     * 
     * @param deltaTime Time elapsed since the last update in seconds
     */
    void update(float deltaTime);

    /**
     * @brief Render the debugger UI
     * 
     * @param width Width of the rendering area
     * @param height Height of the rendering area
     */
    void render(int width, int height);

    /**
     * @brief Set the visibility of the debugger
     * 
     * @param visible True to show the debugger, false to hide it
     */
    void setVisible(bool visible);

    /**
     * @brief Check if the debugger is currently visible
     * 
     * @return bool True if the debugger is visible
     */
    bool isVisible() const;

    /**
     * @brief Set the active CPU architecture for the debugger
     * 
     * @param architecture CPU architecture name (e.g., "m68k", "z80")
     */
    void setArchitecture(const std::string& architecture);

    /**
     * @brief Get the currently active CPU architecture
     * 
     * @return std::string The CPU architecture name
     */
    std::string getArchitecture() const;

    /**
     * @brief Get the memory viewer component
     * 
     * @return MemoryViewer* Pointer to the memory viewer
     */
    MemoryViewer* getMemoryViewer();

    /**
     * @brief Set the memory read callback for the debugger
     * 
     * @param callback Function that reads a byte from memory at the given address
     */
    void setMemoryReadCallback(std::function<uint8_t(uint32_t)> callback);

    /**
     * @brief Set the memory write callback for the debugger
     * 
     * @param callback Function that writes a byte to memory at the given address
     */
    void setMemoryWriteCallback(std::function<void(uint32_t, uint8_t)> callback);

    /**
     * @brief Define a memory region for the debugger
     * 
     * @param name Name of the memory region
     * @param startAddress Start address of the region
     * @param size Size of the region in bytes
     * @param description Optional description of the region
     */
    void defineMemoryRegion(const std::string& name, uint32_t startAddress, 
                          uint32_t size, const std::string& description = "");

    /**
     * @brief Define a structured data type for memory viewing
     * 
     * @param name Name of the structured type
     * @param fields Map of field names to field sizes in bytes
     * @param description Optional description of the structured type
     */
    void defineStructuredType(const std::string& name, 
                            const std::unordered_map<std::string, uint32_t>& fields,
                            const std::string& description = "");

    /**
     * @brief Define a structured view at a specific memory address
     * 
     * @param address Memory address where the structure is located
     * @param typeName Name of the structured type
     * @param instanceName Optional instance name for the structure
     */
    void defineStructuredView(uint32_t address, const std::string& typeName,
                            const std::string& instanceName = "");

    /**
     * @brief Step the CPU execution by one instruction
     */
    void stepInstruction();

    /**
     * @brief Pause the emulation
     */
    void pause();

    /**
     * @brief Resume the emulation
     */
    void resume();

    /**
     * @brief Check if the emulation is currently paused
     * 
     * @return bool True if the emulation is paused
     */
    bool isPaused() const;

    /**
     * @brief Save the debugger settings to a file
     * 
     * @param filename Path to the settings file
     * @return bool True if the settings were saved successfully
     */
    bool saveSettings(const std::string& filename);

    /**
     * @brief Load the debugger settings from a file
     * 
     * @param filename Path to the settings file
     * @return bool True if the settings were loaded successfully
     */
    bool loadSettings(const std::string& filename);

private:
    // Implementation details
    class DebuggerPrivate;
    std::unique_ptr<DebuggerPrivate> m_private;

    // Debugging components
    std::unique_ptr<MemoryViewer> m_memoryViewer;
    std::unique_ptr<Disassembler> m_disassembler;
    std::unique_ptr<BreakpointManager> m_breakpointManager;
    std::unique_ptr<WatchpointManager> m_watchpointManager;
    std::unique_ptr<RegisterViewer> m_registerViewer;

    // State
    bool m_initialized;
    bool m_visible;
    bool m_paused;
    std::string m_architecture;

    // Layouts and UI state
    void layoutComponents(int width, int height);
    void handleInput();
    void renderMainWindow(int width, int height);
    void renderStatusBar(int width, int height);
}; 