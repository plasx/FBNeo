#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <cstdint>
#include <set>
#include "cpu_core.h"

// Forward declarations
class MetalContext;
struct CPUCoreInfo;
class CPUCore;

/**
 * @class DisassemblyViewer
 * @brief Provides disassembly visualization and navigation for debugging
 * 
 * The DisassemblyViewer handles disassembly of memory regions, navigation through
 * disassembled code, breakpoint management, and retrieval of register values
 * for different CPU architectures.
 */
class DisassemblyViewer {
public:
    /**
     * @enum Architecture
     * @brief Supported CPU architectures
     */
    enum class Architecture {
        M68K,
        ARM,
        MIPS,
        Z80,
        M6502,
        UNKNOWN
    };

    /**
     * @brief Information about a disassembled instruction
     */
    struct Instruction {
        uint32_t address;         ///< Memory address of the instruction
        std::string mnemonic;     ///< Instruction mnemonic (e.g., "MOVE")
        std::string operands;     ///< Instruction operands (e.g., "D0, (A0)")
        uint8_t size;             ///< Size of the instruction in bytes
        uint8_t cycles;           ///< Execution cycles of the instruction
        bool hasBreakpoint;       ///< Whether this instruction has a breakpoint
        bool isProgramCounter;    ///< Whether this is the current PC
        std::string comment;      ///< Optional comment for this instruction
    };

    /**
     * @brief Register information for display
     */
    struct Register {
        std::string name;         ///< Register name (e.g., "D0", "A0", "PC")
        uint32_t value;           ///< Current value of the register
        bool isChanged;           ///< Whether the register has changed since last frame
    };

    /**
     * @brief Breakpoint structure
     */
    struct Breakpoint {
        uint32_t address;
        bool enabled;
        std::string condition;
        std::string description;
    };

    /**
     * @brief Constructor
     * @param cpuCore The CPU core to use for disassembly
     */
    DisassemblyViewer(std::shared_ptr<CPUCore> cpuCore);

    /**
     * @brief Destructor
     */
    ~DisassemblyViewer();

    /**
     * @brief Initialize with the specified architecture
     * @param architecture The CPU architecture to use
     * @return True if initialization was successful, false otherwise
     */
    bool initialize(const std::string& architecture);

    /**
     * @brief Initialize with the specified architecture
     * @param architecture The CPU architecture to use
     * @return True if initialization was successful, false otherwise
     */
    bool initialize(Architecture architecture);

    /**
     * @brief Update the disassembly viewer state
     * @param deltaTime Time elapsed since the last update
     */
    void update(float deltaTime);

    /**
     * @brief Render the disassembly viewer
     */
    void render();

    /**
     * @brief Set the current CPU architecture to disassemble
     * @param architecture The CPU architecture to disassemble
     */
    void setArchitecture(Architecture architecture);

    /**
     * @brief Set the CPU core information for disassembly
     * @param coreInfo Pointer to CPU core information structure
     */
    void setCPUCore(const CPUCoreInfo* coreInfo);

    /**
     * @brief Set the current program counter for highlighting
     * @param pc The program counter value
     */
    void setPC(uint32_t pc);

    /**
     * @brief Toggle a breakpoint at the specified address
     * @param address The address to toggle the breakpoint at
     * @return True if a breakpoint was set, false if it was removed
     */
    bool toggleBreakpoint(uint32_t address);

    /**
     * @brief Check if the address has a breakpoint
     * @param address The address to check
     * @return True if the address has a breakpoint, false otherwise
     */
    bool hasBreakpoint(uint32_t address) const;

    /**
     * @brief Set the callback for when execution is paused
     * @param callback Function to call when execution is paused
     */
    void setOnPauseCallback(std::function<void()> callback);

    /**
     * @brief Set the callback for when execution is resumed
     * @param callback Function to call when execution is resumed
     */
    void setOnResumeCallback(std::function<void()> callback);

    /**
     * @brief Set the callback for when step-into is requested
     * @param callback Function to call when step-into is requested
     */
    void setOnStepIntoCallback(std::function<void()> callback);

    /**
     * @brief Set the callback for when step-over is requested
     * @param callback Function to call when step-over is requested
     */
    void setOnStepOverCallback(std::function<void()> callback);

    /**
     * @brief Set the callback for when step-out is requested
     * @param callback Function to call when step-out is requested
     */
    void setOnStepOutCallback(std::function<void()> callback);

    /**
     * @brief Navigate to a specific address in the disassembly view
     * @param address The address to navigate to
     */
    void navigateTo(uint32_t address);

    /**
     * @brief Set whether the disassembly viewer is visible
     * @param visible True to make the viewer visible, false to hide it
     */
    void setVisible(bool visible);

    /**
     * @brief Check if the disassembly viewer is visible
     * @return True if the viewer is visible, false otherwise
     */
    bool isVisible() const;

    /**
     * @brief Set the memory read function for the disassembly viewer
     * @param readFn Function that reads memory at the specified address
     */
    void setMemoryReadFunction(std::function<uint8_t(uint32_t)> readFn);

    /**
     * @brief Set the register update function for the disassembly viewer
     * @param updateFunc Function that updates the register information
     */
    void setRegisterUpdateFunction(std::function<void(std::vector<Register>&)> updateFunc);

    /**
     * @brief Set the execution address (PC)
     * @param address The execution address value
     */
    void setExecutionAddress(uint32_t address);

    /**
     * @brief Disassemble a range of memory
     * @param startAddress The starting address to disassemble
     * @param count The number of instructions to disassemble
     * @return Vector of disassembled instructions
     */
    std::vector<Instruction> disassembleRange(uint32_t startAddress, size_t count) const;

    /**
     * @brief Navigate in the disassembly
     */
    void navigateNext();
    void navigatePrevious();
    void navigateToExecutionPoint();
    void followJump();

    /**
     * @brief Breakpoint management
     */
    void addBreakpoint(uint32_t address, const std::string& condition = "", const std::string& description = "");
    void removeBreakpoint(uint32_t address);
    void enableBreakpoint(uint32_t address, bool enabled);
    void clearAllBreakpoints();
    std::vector<Breakpoint> getBreakpoints() const;

    /**
     * @brief Get register information
     * @return Vector of register names
     */
    std::vector<std::string> getRegisterNames() const;

    /**
     * @brief Analysis helpers
     */
    bool isCallInstruction(const Instruction& instruction) const;
    bool isReturnInstruction(const Instruction& instruction) const;
    bool isJumpInstruction(const Instruction& instruction) const;
    uint32_t getBranchTargetAddress(const Instruction& instruction) const;

    /**
     * @brief Get the string representation of an address based on the architecture
     * @param address The address to format
     * @return Formatted address string
     */
    std::string formatAddress(uint32_t address) const;

    /**
     * @brief Get the current disassembly architecture
     * @return The current disassembly architecture
     */
    Architecture getArchitecture() const;

    /**
     * @brief Convert an architecture string to enum
     * @param archStr Architecture name as string
     * @return Architecture enum value
     */
    static Architecture stringToArchitecture(const std::string& archStr);

    /**
     * @brief Convert an architecture enum to string
     * @param arch Architecture enum value
     * @return Architecture name as string
     */
    static std::string architectureToString(Architecture arch);

    /**
     * @brief Set the current target address
     * @param address The address to navigate to
     */
    void setCurrentAddress(uint32_t address);

    /**
     * @brief Get the current address
     * @return The current target address
     */
    uint32_t getCurrentAddress() const;

    /**
     * @brief Disassemble a range of instructions
     * @param startAddress The starting address
     * @param count The number of instructions to disassemble
     * @return Vector of disassembled instructions
     */
    std::vector<CPUCore::DisassembledInstruction> disassembleRange(uint32_t startAddress, size_t count);

    /**
     * @brief Set a callback for when the current address changes
     * @param callback The function to call when the address changes
     */
    void setAddressChangedCallback(std::function<void(uint32_t)> callback);

    /**
     * @brief Set a callback for when breakpoints change
     * @param callback The function to call when breakpoints change
     */
    void setBreakpointsChangedCallback(std::function<void()> callback);

    /**
     * @brief Render the disassembly view
     * @param x The x position to render at
     * @param y The y position to render at
     * @param width The width of the view
     * @param height The height of the view
     */
    void render(float x, float y, float width, float height);

private:
    class DisassemblyViewerPrivate;
    std::unique_ptr<DisassemblyViewerPrivate> m_private;
    std::shared_ptr<CPUCore> m_cpuCore;                     ///< CPU core used for disassembly
    uint32_t m_currentAddress;                              ///< Current address being viewed
    std::unordered_map<uint32_t, bool> m_breakpoints;       ///< Map of breakpoint addresses
    std::function<void(uint32_t)> m_addressChangedCallback; ///< Callback for address changes
    std::function<void()> m_breakpointsChangedCallback;     ///< Callback for breakpoint changes
    
    /**
     * @brief Find the address of the previous instruction
     * @param currentAddress The current address
     * @return The address of the previous instruction
     */
    uint32_t findPreviousInstructionAddress(uint32_t currentAddress);
};

/**
 * @struct CPUCoreInfo
 * @brief Information about a CPU core for disassembly
 */
struct CPUCoreInfo {
    DisassemblyViewer::Architecture architecture;   ///< CPU architecture
    uint32_t pcRegister;           ///< Current PC register value
    uint32_t spRegister;           ///< Current SP register value
    std::vector<DisassemblyViewer::Register> registers; ///< All CPU registers
    std::function<uint8_t(uint32_t)> memoryRead;   ///< Function to read memory
    std::function<void()> pauseExecution;          ///< Function to pause execution
    std::function<void()> resumeExecution;         ///< Function to resume execution
    std::function<void()> stepInto;                ///< Function to step into next instruction
    std::function<void()> stepOver;                ///< Function to step over current instruction
    std::function<void()> stepOut;                 ///< Function to step out of current function
}; 