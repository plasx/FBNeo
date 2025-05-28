#include "debug_visualizer.h"
#include "memory_viewer.h"
#include "register_viewer.h"
#include "call_stack_viewer.h"
#include "watch_viewer.h"
#include <iostream>

// Constructor
DebugVisualizer::DebugVisualizer()
    : m_state(DebuggerState::Inactive)
    , m_stepOverReturnAddress(0)
    , m_stepOutStackLevel(0)
{
    m_breakpointManager = std::make_shared<BreakpointManager>();
}

// Destructor
DebugVisualizer::~DebugVisualizer()
{
    shutdown();
}

// Initialize the debug visualizer
bool DebugVisualizer::initialize(const std::string& cpuArchitecture)
{
    // Create disassembly viewer for the primary architecture
    if (!addCpuArchitecture(cpuArchitecture)) {
        return false;
    }
    
    m_activeCpuArchitecture = cpuArchitecture;
    m_state = DebuggerState::Running;
    
    // Initialize other components
    m_memoryViewer = std::make_shared<MemoryViewer>();
    m_registerViewer = std::make_shared<RegisterViewer>();
    m_callStackViewer = std::make_shared<CallStackViewer>();
    
    // Initialize WatchViewer with a WatchpointManager
    std::shared_ptr<WatchpointManager> watchpointManager = std::make_shared<WatchpointManager>();
    watchpointManager->initialize();
    m_watchViewer = std::make_shared<WatchViewer>();
    m_watchViewer->initialize(nullptr, watchpointManager);
    
    // Set the architecture for components that need it
    if (m_registerViewer) {
        m_registerViewer->initialize();
        m_registerViewer->setArchitecture(cpuArchitecture);
    }
    
    if (m_callStackViewer) {
        m_callStackViewer->initialize(nullptr, getDisassemblyViewer());
        m_callStackViewer->setArchitecture(cpuArchitecture);
    }
    
    if (m_memoryViewer) {
        m_memoryViewer->initialize(nullptr);
    }
    
    return true;
}

// Shutdown the debug visualizer
void DebugVisualizer::shutdown()
{
    m_disassemblyViewers.clear();
    m_memoryViewer.reset();
    m_registerViewer.reset();
    m_callStackViewer.reset();
    m_watchViewer.reset();
    m_breakpointManager.reset();
    
    m_state = DebuggerState::Inactive;
}

// Update the debug visualizer state
void DebugVisualizer::update(float deltaTime)
{
    // If we're not active, do nothing
    if (m_state == DebuggerState::Inactive) {
        return;
    }
    
    // Check for breakpoints in Running state
    if (m_state == DebuggerState::Running) {
        if (checkBreakpoints()) {
            pauseEmulation();
        }
    }
    
    // Handle stepping operations
    if (m_state == DebuggerState::StepOver || 
        m_state == DebuggerState::StepInto ||
        m_state == DebuggerState::StepOut) {
        
        if (handleStepping()) {
            // Stepping is complete, pause the emulation
            pauseEmulation();
        }
    }
    
    // Update all components
    for (auto& pair : m_disassemblyViewers) {
        pair.second->update(deltaTime);
    }
    
    // Update other components
    if (m_memoryViewer) m_memoryViewer->update(deltaTime);
    if (m_registerViewer) m_registerViewer->update(deltaTime);
    if (m_callStackViewer) m_callStackViewer->update(deltaTime);
    if (m_watchViewer) m_watchViewer->update(deltaTime);
}

// Render the debug visualizer
void DebugVisualizer::render(float x, float y, float width, float height)
{
    // If we're not active, do nothing
    if (m_state == DebuggerState::Inactive) {
        return;
    }
    
    // Render the active disassembly viewer
    auto it = m_disassemblyViewers.find(m_activeCpuArchitecture);
    if (it != m_disassemblyViewers.end()) {
        it->second->render(x, y, width * 0.5f, height * 0.5f);
    }
    
    // Render other components
    if (m_memoryViewer) m_memoryViewer->render(x + width * 0.5f, y, width * 0.5f, height * 0.5f);
    if (m_registerViewer) m_registerViewer->render(x, y + height * 0.5f, width * 0.5f, height * 0.5f);
    if (m_callStackViewer) m_callStackViewer->render(x + width * 0.5f, y + height * 0.5f, width * 0.25f, height * 0.5f);
    if (m_watchViewer) m_watchViewer->render(x + width * 0.75f, y + height * 0.5f, width * 0.25f, height * 0.5f);
}

// Get the current debugger state
DebugVisualizer::DebuggerState DebugVisualizer::getState() const
{
    return m_state;
}

// Set the debugger state
void DebugVisualizer::setState(DebuggerState state)
{
    m_state = state;
}

// Get the disassembly viewer
std::shared_ptr<DisassemblyViewer> DebugVisualizer::getDisassemblyViewer()
{
    auto it = m_disassemblyViewers.find(m_activeCpuArchitecture);
    if (it != m_disassemblyViewers.end()) {
        return it->second;
    }
    return nullptr;
}

// Get the memory viewer
std::shared_ptr<MemoryViewer> DebugVisualizer::getMemoryViewer()
{
    return m_memoryViewer;
}

// Get the register viewer
std::shared_ptr<RegisterViewer> DebugVisualizer::getRegisterViewer()
{
    return m_registerViewer;
}

// Get the call stack viewer
std::shared_ptr<CallStackViewer> DebugVisualizer::getCallStackViewer()
{
    return m_callStackViewer;
}

// Get the watch viewer
std::shared_ptr<WatchViewer> DebugVisualizer::getWatchViewer()
{
    return m_watchViewer;
}

// Get the breakpoint manager
std::shared_ptr<BreakpointManager> DebugVisualizer::getBreakpointManager()
{
    return m_breakpointManager;
}

// Set CPU execution address (PC)
void DebugVisualizer::setExecutionAddress(uint32_t address, const std::string& cpuArchitecture)
{
    auto it = m_disassemblyViewers.find(cpuArchitecture);
    if (it != m_disassemblyViewers.end()) {
        it->second->setExecutionAddress(address);
    }
    
    // Update call stack viewer if this is the active architecture
    if (cpuArchitecture == m_activeCpuArchitecture && m_callStackViewer) {
        m_callStackViewer->setPC(address);
    }
}

// Set memory read function for a CPU architecture
void DebugVisualizer::setMemoryReadFunction(
    const std::string& cpuArchitecture,
    std::function<uint8_t(uint32_t)> readFunction)
{
    // Store the function for the specified architecture
    m_memoryReadFunctions[cpuArchitecture] = readFunction;
    
    // Update the disassembly viewer
    auto it = m_disassemblyViewers.find(cpuArchitecture);
    if (it != m_disassemblyViewers.end()) {
        it->second->setMemoryReadFunction(readFunction);
    }
    
    // If this is the active architecture, update other components
    if (cpuArchitecture == m_activeCpuArchitecture) {
        if (m_memoryViewer) {
            m_memoryViewer->setReadCallback(readFunction);
        }
        if (m_callStackViewer) {
            m_callStackViewer->setMemoryReadCallback(readFunction);
        }
    }
}

// Set memory write function for a CPU architecture
void DebugVisualizer::setMemoryWriteFunction(
    const std::string& cpuArchitecture,
    std::function<void(uint32_t, uint8_t)> writeFunction)
{
    // Store the function for the specified architecture
    m_memoryWriteFunctions[cpuArchitecture] = writeFunction;
    
    // If this is the active architecture, update components
    if (cpuArchitecture == m_activeCpuArchitecture && m_memoryViewer) {
        m_memoryViewer->setWriteCallback(writeFunction);
    }
}

// Add a CPU architecture to the debugger
bool DebugVisualizer::addCpuArchitecture(const std::string& cpuArchitecture)
{
    // Check if already exists
    if (m_disassemblyViewers.find(cpuArchitecture) != m_disassemblyViewers.end()) {
        return true; // Already exists, nothing to do
    }
    
    // Create a new disassembly viewer
    auto disasmViewer = std::make_shared<DisassemblyViewer>(nullptr);
    
    // Initialize with the appropriate architecture
    DisassemblyViewer::Architecture arch;
    
    if (cpuArchitecture == "M68K") {
        arch = DisassemblyViewer::Architecture::M68K;
    } else if (cpuArchitecture == "Z80") {
        arch = DisassemblyViewer::Architecture::Z80;
    } else if (cpuArchitecture == "ARM") {
        arch = DisassemblyViewer::Architecture::ARM;
    } else if (cpuArchitecture == "MIPS") {
        arch = DisassemblyViewer::Architecture::MIPS;
    } else if (cpuArchitecture == "M6502") {
        arch = DisassemblyViewer::Architecture::M6502;
    } else {
        std::cerr << "Unsupported CPU architecture: " << cpuArchitecture << std::endl;
        return false;
    }
    
    disasmViewer->initialize(arch);
    
    // Add to our collection
    m_disassemblyViewers[cpuArchitecture] = disasmViewer;
    
    return true;
}

// Set active CPU architecture
bool DebugVisualizer::setActiveCpuArchitecture(const std::string& cpuArchitecture)
{
    // Check if the architecture exists
    auto it = m_disassemblyViewers.find(cpuArchitecture);
    if (it == m_disassemblyViewers.end()) {
        return false;
    }
    
    m_activeCpuArchitecture = cpuArchitecture;
    
    // Update other components as needed
    if (m_registerViewer) {
        m_registerViewer->setArchitecture(cpuArchitecture);
    }
    
    if (m_callStackViewer) {
        m_callStackViewer->setArchitecture(cpuArchitecture);
    }
    
    if (m_watchViewer) {
        m_watchViewer->setArchitecture(cpuArchitecture);
    }
    
    auto readFnIt = m_memoryReadFunctions.find(cpuArchitecture);
    if (readFnIt != m_memoryReadFunctions.end() && m_memoryViewer) {
        m_memoryViewer->setReadCallback(readFnIt->second);
    }
    
    auto writeFnIt = m_memoryWriteFunctions.find(cpuArchitecture);
    if (writeFnIt != m_memoryWriteFunctions.end() && m_memoryViewer) {
        m_memoryViewer->setWriteCallback(writeFnIt->second);
    }
    
    return true;
}

// Get the active CPU architecture
std::string DebugVisualizer::getActiveCpuArchitecture() const
{
    return m_activeCpuArchitecture;
}

// Check if a breakpoint is hit at the current execution address
bool DebugVisualizer::checkBreakpoints()
{
    if (!m_breakpointManager) {
        return false;
    }
    
    // Get the active disassembly viewer
    auto it = m_disassemblyViewers.find(m_activeCpuArchitecture);
    if (it == m_disassemblyViewers.end()) {
        return false;
    }
    
    // Get the current execution address
    uint32_t address = it->second->getExecutionAddress();
    
    // Check if there's a breakpoint at this address
    return m_breakpointManager->shouldBreak(address, m_activeCpuArchitecture);
}

// Handle stepping (over, into, out)
bool DebugVisualizer::handleStepping()
{
    if (m_state == DebuggerState::StepOver) {
        // Get the active disassembly viewer
        auto it = m_disassemblyViewers.find(m_activeCpuArchitecture);
        if (it == m_disassemblyViewers.end()) {
            return true; // Can't step, so consider it done
        }
        
        // Get the current execution address
        uint32_t address = it->second->getExecutionAddress();
        
        // If we've reached the return address, we're done stepping
        return address == m_stepOverReturnAddress;
    }
    else if (m_state == DebuggerState::StepInto) {
        // Step into always completes after a single instruction
        return true;
    }
    else if (m_state == DebuggerState::StepOut) {
        // If call stack viewer is available, use its depth
        if (m_callStackViewer && m_callStackViewer->getCallStackDepth() <= m_stepOutStackLevel) {
            return true;  // We've returned to the caller's frame or higher
        }
        
        // Without call stack info, we can't really implement step out
        // For now, we'll just return true to complete the step out
        return true;
    }
    
    return false;
}

// Pause the emulation
void DebugVisualizer::pauseEmulation()
{
    m_state = DebuggerState::Paused;
    
    // In a real implementation, we would need to signal the emulation to pause
    // This might involve setting a flag or sending a message to the emulation thread
}

// Resume the emulation
void DebugVisualizer::resumeEmulation()
{
    m_state = DebuggerState::Running;
    
    // In a real implementation, we would need to signal the emulation to resume
    // This might involve setting a flag or sending a message to the emulation thread
}

// Step over the current instruction
void DebugVisualizer::stepOver()
{
    // Get the active disassembly viewer
    auto it = m_disassemblyViewers.find(m_activeCpuArchitecture);
    if (it == m_disassemblyViewers.end()) {
        return;
    }
    
    // Get the current execution address
    uint32_t address = it->second->getExecutionAddress();
    
    // Get the current instruction
    auto instructions = it->second->disassembleRange(address, 1);
    if (instructions.empty()) {
        return;
    }
    
    // Set the return address to the next instruction
    m_stepOverReturnAddress = address + instructions[0].size;
    
    // Change the state to StepOver
    m_state = DebuggerState::StepOver;
    
    // Resume the emulation
    resumeEmulation();
}

// Step into the current instruction (if it's a call)
void DebugVisualizer::stepInto()
{
    // Change the state to StepInto
    m_state = DebuggerState::StepInto;
    
    // If the call stack viewer is available, we can track call/returns
    if (m_callStackViewer && !m_callStackViewer->getCallStack().empty()) {
        // Store the current call stack depth for later comparison
        m_stepOutStackLevel = m_callStackViewer->getCallStackDepth();
    }
    
    // Resume the emulation for a single step
    resumeEmulation();
}

// Step out of the current function
void DebugVisualizer::stepOut()
{
    // If call stack viewer is available, use its depth
    if (m_callStackViewer) {
        m_stepOutStackLevel = m_callStackViewer->getCallStackDepth();
        if (m_stepOutStackLevel > 0) {
            m_stepOutStackLevel--;  // We want to return to the caller's frame
        }
    } else {
        m_stepOutStackLevel = 0;
    }
    
    // Change the state to StepOut
    m_state = DebuggerState::StepOut;
    
    // Resume the emulation
    resumeEmulation();
}

// Toggle a breakpoint at the specified address
bool DebugVisualizer::toggleBreakpoint(uint32_t address, const std::string& cpuArchitecture)
{
    if (!m_breakpointManager) {
        return false;
    }
    
    // Check if a breakpoint already exists
    bool hasBreakpoint = m_breakpointManager->hasBreakpoint(address, cpuArchitecture);
    
    if (hasBreakpoint) {
        // Find the breakpoint ID and remove it
        for (const auto& pair : m_breakpointManager->getAllBreakpoints()) {
            if (pair.second.address == address && pair.second.architecture == cpuArchitecture) {
                m_breakpointManager->removeBreakpoint(pair.first);
                return false; // Breakpoint removed
            }
        }
        return false; // Shouldn't reach here, but return false just in case
    }
    else {
        // Add a new breakpoint
        m_breakpointManager->addBreakpoint(address, cpuArchitecture);
        return true; // Breakpoint added
    }
}

// Set register values for a CPU architecture
void DebugVisualizer::setRegisterValues(
    const std::string& cpuArchitecture,
    const std::unordered_map<std::string, uint64_t>& registers)
{
    // If this is the active architecture, update the register viewer
    if (cpuArchitecture == m_activeCpuArchitecture && m_registerViewer) {
        m_registerViewer->updateRegisters(registers);
    }
    
    // Also update stack pointers for call stack viewer if available
    if (cpuArchitecture == m_activeCpuArchitecture && m_callStackViewer) {
        // Look for architecture-specific stack and frame pointers
        if (cpuArchitecture == "M68K") {
            // M68K: A7 is SP, A6 is often FP
            auto spIt = registers.find("A7");
            if (spIt != registers.end()) {
                m_callStackViewer->setStackPointer(static_cast<uint32_t>(spIt->second));
            }
            auto fpIt = registers.find("A6");
            if (fpIt != registers.end()) {
                m_callStackViewer->setFramePointer(static_cast<uint32_t>(fpIt->second));
            }
        } else if (cpuArchitecture == "Z80") {
            // Z80: SP register
            auto spIt = registers.find("SP");
            if (spIt != registers.end()) {
                m_callStackViewer->setStackPointer(static_cast<uint32_t>(spIt->second));
            }
        } else if (cpuArchitecture == "ARM") {
            // ARM: R13 is SP, R11 is FP
            auto spIt = registers.find("R13");
            if (spIt != registers.end()) {
                m_callStackViewer->setStackPointer(static_cast<uint32_t>(spIt->second));
            }
            auto fpIt = registers.find("R11");
            if (fpIt != registers.end()) {
                m_callStackViewer->setFramePointer(static_cast<uint32_t>(fpIt->second));
            }
        } else if (cpuArchitecture == "MIPS") {
            // MIPS: $29 is SP, $30 is FP
            auto spIt = registers.find("$29");
            if (spIt != registers.end()) {
                m_callStackViewer->setStackPointer(static_cast<uint32_t>(spIt->second));
            }
            auto fpIt = registers.find("$30");
            if (fpIt != registers.end()) {
                m_callStackViewer->setFramePointer(static_cast<uint32_t>(fpIt->second));
            }
        } else if (cpuArchitecture == "M6502") {
            // 6502: S register is SP
            auto spIt = registers.find("S");
            if (spIt != registers.end()) {
                m_callStackViewer->setStackPointer(static_cast<uint32_t>(spIt->second));
            }
        }
    }
} 