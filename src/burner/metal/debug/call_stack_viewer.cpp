#include "call_stack_viewer.h"
#include "disassembly_viewer.h"
#include <iostream>
#include <iomanip>
#include <algorithm>

// Constructor
CallStackViewer::CallStackViewer()
    : m_metalContext(nullptr)
    , m_disassemblyViewer(nullptr)
    , m_architecture("")
    , m_stackPointer(0)
    , m_framePointer(0)
    , m_pc(0)
    , m_maxDisplayDepth(32)
    , m_autoStackDetection(true)
    , m_memoryReadCallback(nullptr)
    , m_symbolLookupCallback(nullptr)
{
}

// Destructor
CallStackViewer::~CallStackViewer()
{
}

// Initialize the call stack viewer
bool CallStackViewer::initialize(MetalContext* metalContext, std::shared_ptr<DisassemblyViewer> disassemblyViewer)
{
    m_metalContext = metalContext;
    m_disassemblyViewer = disassemblyViewer;
    m_callStack.clear();
    
    return true;
}

// Update the call stack viewer
void CallStackViewer::update(float deltaTime)
{
    // If automatic stack detection is enabled, detect stack frames
    if (m_autoStackDetection && m_memoryReadCallback) {
        detectStackFrames();
    }
}

// Render the call stack viewer
void CallStackViewer::render(float x, float y, float width, float height)
{
    if (!m_metalContext) {
        return;
    }
    
    // In a real implementation, this would use a UI library like ImGui
    // For now, we'll just output to the console as a placeholder
    
    std::cout << "CallStackViewer rendering at (" << x << ", " << y << ") with size " 
              << width << "x" << height << std::endl;
    
    if (m_callStack.empty()) {
        std::cout << "  Call stack is empty." << std::endl;
        return;
    }
    
    // Print header
    std::cout << "  " 
              << std::setw(4) << "#" << " | "
              << std::setw(10) << "Caller" << " | "
              << std::setw(10) << "Return" << " | "
              << std::setw(10) << "Current" << " | "
              << "Function" << std::endl;
    
    std::cout << "  " << std::string(80, '-') << std::endl;
    
    // Determine how many frames to display
    size_t displayCount = std::min(m_callStack.size(), m_maxDisplayDepth);
    
    // Print frames
    for (size_t i = 0; i < displayCount; ++i) {
        const auto& frame = m_callStack[i];
        
        bool isCurrentFrame = (i == 0); // Top of stack is current frame
        std::string prefix = isCurrentFrame ? "▶ " : "  ";
        
        std::cout << prefix 
                  << std::setw(2) << i << " | "
                  << "0x" << std::hex << std::setw(8) << std::setfill('0') << frame.callAddress << std::dec << std::setfill(' ') << " | "
                  << "0x" << std::hex << std::setw(8) << std::setfill('0') << frame.returnAddress << std::dec << std::setfill(' ') << " | "
                  << "0x" << std::hex << std::setw(8) << std::setfill('0') << frame.currentAddress << std::dec << std::setfill(' ') << " | "
                  << frame.functionName << std::endl;
    }
    
    // If there are more frames than we displayed, show a message
    if (m_callStack.size() > m_maxDisplayDepth) {
        std::cout << "  ... " << (m_callStack.size() - m_maxDisplayDepth) << " more frames ..." << std::endl;
    }
}

// Set the current CPU architecture
void CallStackViewer::setArchitecture(const std::string& architecture)
{
    if (m_architecture != architecture) {
        m_architecture = architecture;
        clearStack(); // Clear stack when architecture changes
    }
}

// Set the stack pointer register value
void CallStackViewer::setStackPointer(uint32_t stackPointer)
{
    m_stackPointer = stackPointer;
}

// Set the frame pointer register value
void CallStackViewer::setFramePointer(uint32_t framePointer)
{
    m_framePointer = framePointer;
}

// Set the current program counter
void CallStackViewer::setPC(uint32_t pc)
{
    m_pc = pc;
    
    // Update the current address of the top stack frame
    if (!m_callStack.empty()) {
        m_callStack[0].currentAddress = pc;
    }
}

// Set the memory read callback
void CallStackViewer::setMemoryReadCallback(std::function<uint8_t(uint32_t)> callback)
{
    m_memoryReadCallback = callback;
}

// Set the symbol lookup callback
void CallStackViewer::setSymbolLookupCallback(std::function<std::string(uint32_t)> callback)
{
    m_symbolLookupCallback = callback;
}

// Push a call onto the stack
void CallStackViewer::pushCall(uint32_t callAddress, uint32_t returnAddress, const std::string& functionName)
{
    StackFrame frame;
    frame.callAddress = callAddress;
    frame.returnAddress = returnAddress;
    frame.currentAddress = m_pc;
    frame.functionName = functionName.empty() && m_symbolLookupCallback ? 
                         m_symbolLookupCallback(callAddress) : functionName;
    frame.stackPointer = m_stackPointer;
    frame.framePointer = m_framePointer;
    
    m_callStack.insert(m_callStack.begin(), frame);
}

// Pop a call from the stack
bool CallStackViewer::popCall()
{
    if (m_callStack.empty()) {
        return false;
    }
    
    m_callStack.erase(m_callStack.begin());
    return true;
}

// Clear the call stack
void CallStackViewer::clearStack()
{
    m_callStack.clear();
}

// Get the current call stack
std::vector<CallStackViewer::StackFrame> CallStackViewer::getCallStack() const
{
    return m_callStack;
}

// Get the current call stack depth
size_t CallStackViewer::getCallStackDepth() const
{
    return m_callStack.size();
}

// Set the maximum call stack depth to display
void CallStackViewer::setMaxDisplayDepth(size_t maxDepth)
{
    m_maxDisplayDepth = maxDepth;
}

// Enable or disable automatic stack detection
void CallStackViewer::setAutoStackDetection(bool enable)
{
    m_autoStackDetection = enable;
}

// Get whether automatic stack detection is enabled
bool CallStackViewer::isAutoStackDetectionEnabled() const
{
    return m_autoStackDetection;
}

// Navigate to a specific frame in the disassembly viewer
bool CallStackViewer::navigateToFrame(size_t frameIndex)
{
    if (!m_disassemblyViewer || frameIndex >= m_callStack.size()) {
        return false;
    }
    
    const auto& frame = m_callStack[frameIndex];
    m_disassemblyViewer->goToAddress(frame.callAddress);
    
    return true;
}

// Helper method to read a 16-bit word from memory
uint16_t CallStackViewer::readWord(uint32_t address) const
{
    if (!m_memoryReadCallback) {
        return 0;
    }
    
    uint16_t lo = m_memoryReadCallback(address);
    uint16_t hi = m_memoryReadCallback(address + 1);
    
    // Adjust byte order based on architecture
    if (m_architecture == "M68K" || m_architecture == "ARM" || m_architecture == "MIPS") {
        // Big endian
        return (hi | (lo << 8));
    } else {
        // Little endian
        return (lo | (hi << 8));
    }
}

// Helper method to read a 32-bit dword from memory
uint32_t CallStackViewer::readDWord(uint32_t address) const
{
    if (!m_memoryReadCallback) {
        return 0;
    }
    
    uint32_t b0 = m_memoryReadCallback(address);
    uint32_t b1 = m_memoryReadCallback(address + 1);
    uint32_t b2 = m_memoryReadCallback(address + 2);
    uint32_t b3 = m_memoryReadCallback(address + 3);
    
    // Adjust byte order based on architecture
    if (m_architecture == "M68K" || m_architecture == "ARM" || m_architecture == "MIPS") {
        // Big endian
        return (b3 | (b2 << 8) | (b1 << 16) | (b0 << 24));
    } else {
        // Little endian
        return (b0 | (b1 << 8) | (b2 << 16) | (b3 << 24));
    }
}

// Detect stack frames based on the current architecture
void CallStackViewer::detectStackFrames()
{
    if (m_architecture == "M68K") {
        detectM68KStackFrames();
    } else if (m_architecture == "Z80") {
        detectZ80StackFrames();
    } else if (m_architecture == "ARM") {
        detectARMStackFrames();
    } else if (m_architecture == "MIPS") {
        detectMIPSStackFrames();
    } else if (m_architecture == "M6502") {
        detectM6502StackFrames();
    }
}

// Architecture-specific stack detection methods
void CallStackViewer::detectM68KStackFrames()
{
    // This is a simplified implementation
    // Real implementation would need to analyze stack frames based on 
    // M68K calling conventions and available debug information
    
    // For now, clear existing stack and create a dummy frame for the current context
    clearStack();
    
    StackFrame frame;
    frame.callAddress = 0; // Unknown
    frame.returnAddress = 0; // Unknown
    frame.currentAddress = m_pc;
    frame.functionName = m_symbolLookupCallback ? m_symbolLookupCallback(m_pc) : "";
    frame.stackPointer = m_stackPointer;
    frame.framePointer = m_framePointer;
    
    m_callStack.push_back(frame);
    
    // In a real implementation, we would:
    // 1. Use the stack pointer and frame pointer to walk the stack
    // 2. For each frame, identify the return address and function
    // 3. Add frames to the call stack
    
    if (m_memoryReadCallback && m_framePointer) {
        uint32_t currentFP = m_framePointer;
        const uint32_t maxFrames = 32; // Prevent infinite loops
        uint32_t frameCount = 1; // We already have one frame
        
        while (currentFP && frameCount < maxFrames) {
            // M68K frames typically have saved FP at [fp] and return address at [fp+4]
            uint32_t savedFP = readDWord(currentFP);
            uint32_t returnAddr = readDWord(currentFP + 4);
            
            // Validate addresses
            if (savedFP == 0 || returnAddr == 0 || 
                savedFP == currentFP || 
                savedFP < 0x1000 || returnAddr < 0x1000) {
                break;
            }
            
            // Create a new frame
            StackFrame newFrame;
            newFrame.callAddress = returnAddr - 4; // Approximate call instruction address
            newFrame.returnAddress = returnAddr;
            newFrame.currentAddress = returnAddr;
            newFrame.functionName = m_symbolLookupCallback ? m_symbolLookupCallback(newFrame.callAddress) : "";
            newFrame.stackPointer = 0; // Unknown
            newFrame.framePointer = currentFP;
            
            m_callStack.push_back(newFrame);
            
            // Move to the previous frame
            currentFP = savedFP;
            frameCount++;
        }
    }
}

void CallStackViewer::detectZ80StackFrames()
{
    // Simplified implementation for Z80 - stack backtracing is more complex
    // due to lack of frame pointers and architecture design
    clearStack();
    
    StackFrame frame;
    frame.callAddress = 0; // Unknown
    frame.returnAddress = 0; // Unknown
    frame.currentAddress = m_pc;
    frame.functionName = m_symbolLookupCallback ? m_symbolLookupCallback(m_pc) : "";
    frame.stackPointer = m_stackPointer;
    frame.framePointer = 0; // Z80 doesn't have a dedicated frame pointer
    
    m_callStack.push_back(frame);
    
    // For Z80, we could look at the stack for return addresses, but this would be very heuristic
    // since there's no standard calling convention that guarantees stack layout
}

void CallStackViewer::detectARMStackFrames()
{
    // ARM stack frame detection would depend on the specific calling convention
    // (ARM, Thumb, AAPCS, etc.) and compiler optimizations
    clearStack();
    
    StackFrame frame;
    frame.callAddress = 0; // Unknown
    frame.returnAddress = 0; // Unknown
    frame.currentAddress = m_pc;
    frame.functionName = m_symbolLookupCallback ? m_symbolLookupCallback(m_pc) : "";
    frame.stackPointer = m_stackPointer;
    frame.framePointer = m_framePointer; // r11 in many ARM calling conventions
    
    m_callStack.push_back(frame);
    
    // A real implementation would use link register (r14) and stack analysis
    // to reconstruct the call stack
}

void CallStackViewer::detectMIPSStackFrames()
{
    // MIPS stack frame detection would vary based on calling convention and compiler
    clearStack();
    
    StackFrame frame;
    frame.callAddress = 0; // Unknown
    frame.returnAddress = 0; // Unknown
    frame.currentAddress = m_pc;
    frame.functionName = m_symbolLookupCallback ? m_symbolLookupCallback(m_pc) : "";
    frame.stackPointer = m_stackPointer;
    frame.framePointer = m_framePointer;
    
    m_callStack.push_back(frame);
    
    // A real implementation would analyze stack based on MIPS calling convention
    // including ra (return address) register tracking
}

void CallStackViewer::detectM6502StackFrames()
{
    // 6502 doesn't have sophisticated call stack mechanisms
    // Just a single byte stack pointer and simple JSR/RTS
    clearStack();
    
    StackFrame frame;
    frame.callAddress = 0; // Unknown
    frame.returnAddress = 0; // Unknown
    frame.currentAddress = m_pc;
    frame.functionName = m_symbolLookupCallback ? m_symbolLookupCallback(m_pc) : "";
    frame.stackPointer = m_stackPointer;
    frame.framePointer = 0; // 6502 doesn't have a frame pointer
    
    m_callStack.push_back(frame);
    
    // For 6502, we could scan the stack for return addresses
    // Each JSR pushes PC+2 onto the stack (low byte first)
    if (m_memoryReadCallback && m_stackPointer < 0xFF) {
        uint32_t sp = m_stackPointer;
        const uint32_t maxFrames = 8; // Prevent excessive scanning
        uint32_t frameCount = 1; // We already have one frame
        
        while (sp < 0xFF && frameCount < maxFrames) {
            // Find potential return address on stack (low byte then high byte)
            uint8_t lo = m_memoryReadCallback(0x100 + sp + 1);
            uint8_t hi = m_memoryReadCallback(0x100 + sp + 2);
            uint16_t returnAddr = (hi << 8) | lo;
            
            // Validate address (crude check)
            if (returnAddr < 0x0800 || returnAddr > 0xFFFF) {
                break;
            }
            
            // Create a new frame
            StackFrame newFrame;
            newFrame.callAddress = returnAddr - 3; // JSR is 3 bytes
            newFrame.returnAddress = returnAddr;
            newFrame.currentAddress = returnAddr;
            newFrame.functionName = m_symbolLookupCallback ? m_symbolLookupCallback(newFrame.callAddress) : "";
            newFrame.stackPointer = sp + 2; // Adjust for the 2 bytes we just read
            newFrame.framePointer = 0;
            
            m_callStack.push_back(newFrame);
            
            // Move to next potential return address
            sp += 2;
            frameCount++;
        }
    }
} 