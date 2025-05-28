#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <cstdint>

// Forward declarations
class MetalContext;
class DisassemblyViewer;

/**
 * @class CallStackViewer
 * @brief Provides visualization of the CPU call stack for debugging
 * 
 * The CallStackViewer tracks function calls and returns in the emulated CPU,
 * allowing developers to visualize the current call stack depth and navigate
 * through the call hierarchy during debugging.
 */
class CallStackViewer {
public:
    /**
     * @struct StackFrame
     * @brief Represents a single frame in the call stack
     */
    struct StackFrame {
        uint32_t callAddress;     ///< Address of the call instruction
        uint32_t returnAddress;   ///< Return address
        uint32_t currentAddress;  ///< Current execution address within this frame
        std::string functionName; ///< Function name if available
        uint32_t stackPointer;    ///< Stack pointer value at this frame
        uint32_t framePointer;    ///< Frame pointer value if available
    };

public:
    /**
     * @brief Constructor
     */
    CallStackViewer();
    
    /**
     * @brief Destructor
     */
    ~CallStackViewer();
    
    /**
     * @brief Initialize the call stack viewer
     * @param metalContext Pointer to the Metal rendering context
     * @param disassemblyViewer Pointer to the disassembly viewer (optional)
     * @return True if initialization was successful, false otherwise
     */
    bool initialize(MetalContext* metalContext, std::shared_ptr<DisassemblyViewer> disassemblyViewer = nullptr);
    
    /**
     * @brief Update the call stack viewer
     * @param deltaTime Time elapsed since last update
     */
    void update(float deltaTime);
    
    /**
     * @brief Render the call stack viewer
     * @param x X-coordinate for rendering
     * @param y Y-coordinate for rendering
     * @param width Width of the rendering area
     * @param height Height of the rendering area
     */
    void render(float x, float y, float width, float height);
    
    /**
     * @brief Set the current CPU architecture
     * @param architecture The CPU architecture (e.g., "M68K", "Z80")
     */
    void setArchitecture(const std::string& architecture);
    
    /**
     * @brief Set the stack pointer register value
     * @param stackPointer The current stack pointer
     */
    void setStackPointer(uint32_t stackPointer);
    
    /**
     * @brief Set the frame pointer register value
     * @param framePointer The current frame pointer
     */
    void setFramePointer(uint32_t framePointer);
    
    /**
     * @brief Set the current program counter
     * @param pc The current program counter value
     */
    void setPC(uint32_t pc);
    
    /**
     * @brief Set the memory read callback
     * @param callback Function that reads a byte from memory
     */
    void setMemoryReadCallback(std::function<uint8_t(uint32_t)> callback);
    
    /**
     * @brief Set the symbol lookup callback
     * @param callback Function that looks up a symbol name from an address
     */
    void setSymbolLookupCallback(std::function<std::string(uint32_t)> callback);
    
    /**
     * @brief Push a call onto the stack
     * @param callAddress The address of the call instruction
     * @param returnAddress The return address
     * @param functionName The function name if available
     */
    void pushCall(uint32_t callAddress, uint32_t returnAddress, const std::string& functionName = "");
    
    /**
     * @brief Pop a call from the stack
     * @return True if a call was popped, false if the stack was empty
     */
    bool popCall();
    
    /**
     * @brief Clear the call stack
     */
    void clearStack();
    
    /**
     * @brief Get the current call stack
     * @return Vector of stack frames
     */
    std::vector<StackFrame> getCallStack() const;
    
    /**
     * @brief Get the current call stack depth
     * @return Number of frames in the stack
     */
    size_t getCallStackDepth() const;
    
    /**
     * @brief Set the maximum call stack depth to display
     * @param maxDepth Maximum number of frames to display
     */
    void setMaxDisplayDepth(size_t maxDepth);
    
    /**
     * @brief Enable or disable automatic stack detection
     * @param enable True to enable, false to disable
     */
    void setAutoStackDetection(bool enable);
    
    /**
     * @brief Get whether automatic stack detection is enabled
     * @return True if enabled, false otherwise
     */
    bool isAutoStackDetectionEnabled() const;
    
    /**
     * @brief Navigate to a specific frame in the disassembly viewer
     * @param frameIndex Index of the frame to navigate to
     * @return True if navigation was successful, false otherwise
     */
    bool navigateToFrame(size_t frameIndex);

private:
    MetalContext* m_metalContext;
    std::shared_ptr<DisassemblyViewer> m_disassemblyViewer;
    
    std::string m_architecture;
    uint32_t m_stackPointer;
    uint32_t m_framePointer;
    uint32_t m_pc;
    
    std::vector<StackFrame> m_callStack;
    size_t m_maxDisplayDepth;
    bool m_autoStackDetection;
    
    std::function<uint8_t(uint32_t)> m_memoryReadCallback;
    std::function<std::string(uint32_t)> m_symbolLookupCallback;
    
    void detectStackFrames();
    uint16_t readWord(uint32_t address) const;
    uint32_t readDWord(uint32_t address) const;
    
    // Architecture-specific stack detection methods
    void detectM68KStackFrames();
    void detectZ80StackFrames();
    void detectARMStackFrames();
    void detectMIPSStackFrames();
    void detectM6502StackFrames();
}; 