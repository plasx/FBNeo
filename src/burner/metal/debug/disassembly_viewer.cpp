#include "disassembly_viewer.h"
#include "cpu_core.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <iostream>

// Constructor
DisassemblyViewer::DisassemblyViewer(std::shared_ptr<CPUCore> cpuCore)
    : m_cpuCore(cpuCore)
    , m_currentAddress(0)
    , m_executionAddress(0)
    , m_historyPosition(0)
    , m_addressChangedCallback(nullptr)
    , m_breakpointsChangedCallback(nullptr)
{
    // Set a default memory read function that returns 0
    m_memoryReadFunc = [](uint32_t) { return 0; };
}

// Destructor
DisassemblyViewer::~DisassemblyViewer()
{
}

// Initialize with a specific architecture
void DisassemblyViewer::initialize(Architecture architecture)
{
    m_architecture = architecture;
    initializeCPUCore();
    m_currentAddress = 0;
    m_executionAddress = 0;
    m_breakpoints.clear();
    m_navigationHistory.clear();
    m_historyPosition = 0;
}

// Initialize the appropriate CPU core
void DisassemblyViewer::initializeCPUCore()
{
    m_cpuCore = CPUCore::createForArchitecture(architectureToString(m_architecture));
}

// Set the memory read function
void DisassemblyViewer::setMemoryReadFunction(std::function<uint8_t(uint32_t)> memoryRead)
{
    m_memoryReadFunc = memoryRead;
    if (m_cpuCore) {
        m_cpuCore->setMemoryReadFunction(memoryRead);
    }
}

// Set the execution address (PC)
void DisassemblyViewer::setExecutionAddress(uint32_t address)
{
    m_executionAddress = address;
}

// Disassemble a range of memory
std::vector<DisassemblyViewer::Instruction> DisassemblyViewer::disassembleRange(
    uint32_t startAddress, size_t count) const
{
    std::vector<Instruction> instructions;
    
    if (!m_cpuCore) {
        return instructions;
    }
    
    uint32_t currentAddress = startAddress;
    
    for (size_t i = 0; i < count; i++) {
        CPUCore::DisassembledInstruction coreInst = m_cpuCore->disassemble(currentAddress);
        
        Instruction inst;
        inst.address = currentAddress;
        inst.mnemonic = coreInst.mnemonic;
        inst.operands = coreInst.operands;
        inst.size = coreInst.size;
        inst.cycles = 0; // Not easily determined without more info
        inst.hasBreakpoint = hasBreakpoint(currentAddress);
        inst.isProgramCounter = (currentAddress == m_executionAddress);
        
        instructions.push_back(inst);
        
        // Move to the next instruction
        currentAddress += coreInst.size;
    }
    
    return instructions;
}

// Navigate in the disassembly
void DisassemblyViewer::navigateTo(uint32_t address)
{
    // Add to navigation history if it's a new address
    if (m_currentAddress != address) {
        if (m_navigationHistory.empty() || m_navigationHistory.back() != m_currentAddress) {
            // If we're not at the end of history, truncate it
            if (m_historyPosition < m_navigationHistory.size()) {
                m_navigationHistory.resize(m_historyPosition + 1);
            }
            m_navigationHistory.push_back(m_currentAddress);
            m_historyPosition = m_navigationHistory.size();
        }
        m_currentAddress = address;
    }
}

void DisassemblyViewer::navigateNext()
{
    if (!m_cpuCore) {
        return;
    }
    
    // Get the current instruction size
    CPUCore::DisassembledInstruction inst = m_cpuCore->disassemble(m_currentAddress);
    
    // Move to the next instruction
    navigateTo(m_currentAddress + inst.size);
}

void DisassemblyViewer::navigatePrevious()
{
    // Finding the previous instruction is tricky since instructions have variable sizes
    // For simplicity, try scanning back a few bytes to find an instruction that leads to current
    
    const int MAX_INSTRUCTION_SIZE = 8; // Most CPUs have a max instruction size
    
    uint32_t targetAddress = m_currentAddress;
    uint32_t bestPrevious = (m_currentAddress > MAX_INSTRUCTION_SIZE) ? 
        m_currentAddress - MAX_INSTRUCTION_SIZE : 0;
    
    for (int i = 1; i <= MAX_INSTRUCTION_SIZE; i++) {
        if (m_currentAddress < i) {
            continue;
        }
        
        uint32_t testAddress = m_currentAddress - i;
        CPUCore::DisassembledInstruction inst = m_cpuCore->disassemble(testAddress);
        
        if (testAddress + inst.size == m_currentAddress) {
            bestPrevious = testAddress;
            break;
        }
    }
    
    navigateTo(bestPrevious);
}

void DisassemblyViewer::navigateToExecutionPoint()
{
    navigateTo(m_executionAddress);
}

void DisassemblyViewer::followJump()
{
    if (!m_cpuCore) {
        return;
    }
    
    CPUCore::DisassembledInstruction inst = m_cpuCore->disassemble(m_currentAddress);
    
    // Only follow if this is a branch/jump instruction
    if (m_cpuCore->isJumpInstruction(inst.mnemonic, inst.operands) ||
        m_cpuCore->isCallInstruction(inst.mnemonic, inst.operands)) {
        
        uint32_t targetAddress = getBranchTargetAddress(
            {m_currentAddress, inst.mnemonic, inst.operands, inst.size});
        
        if (targetAddress != 0) {
            navigateTo(targetAddress);
        }
    }
}

// Breakpoint management
void DisassemblyViewer::addBreakpoint(uint32_t address, const std::string& condition, const std::string& description)
{
    Breakpoint bp;
    bp.address = address;
    bp.enabled = true;
    bp.condition = condition;
    bp.description = description;
    
    m_breakpoints[address] = bp;
}

void DisassemblyViewer::removeBreakpoint(uint32_t address)
{
    m_breakpoints.erase(address);
}

void DisassemblyViewer::enableBreakpoint(uint32_t address, bool enabled)
{
    auto it = m_breakpoints.find(address);
    if (it != m_breakpoints.end()) {
        it->second.enabled = enabled;
    }
}

void DisassemblyViewer::clearAllBreakpoints()
{
    m_breakpoints.clear();
}

std::vector<DisassemblyViewer::Breakpoint> DisassemblyViewer::getBreakpoints() const
{
    std::vector<Breakpoint> result;
    for (const auto& pair : m_breakpoints) {
        result.push_back(pair.second);
    }
    return result;
}

bool DisassemblyViewer::hasBreakpoint(uint32_t address) const
{
    return m_breakpoints.find(address) != m_breakpoints.end();
}

// Get register information
std::vector<std::string> DisassemblyViewer::getRegisterNames() const
{
    if (m_cpuCore) {
        return m_cpuCore->getRegisterNames();
    }
    return {};
}

// Analysis helpers
bool DisassemblyViewer::isCallInstruction(const Instruction& instruction) const
{
    if (m_cpuCore) {
        return m_cpuCore->isCallInstruction(instruction.mnemonic, instruction.operands);
    }
    return false;
}

bool DisassemblyViewer::isReturnInstruction(const Instruction& instruction) const
{
    if (m_cpuCore) {
        return m_cpuCore->isReturnInstruction(instruction.mnemonic, instruction.operands);
    }
    return false;
}

bool DisassemblyViewer::isJumpInstruction(const Instruction& instruction) const
{
    if (m_cpuCore) {
        return m_cpuCore->isJumpInstruction(instruction.mnemonic, instruction.operands);
    }
    return false;
}

uint32_t DisassemblyViewer::getBranchTargetAddress(const Instruction& instruction) const
{
    if (m_cpuCore) {
        return m_cpuCore->getTargetAddress(instruction.address, instruction.mnemonic, instruction.operands);
    }
    return 0;
}

// Get the string representation of an address based on the architecture
std::string DisassemblyViewer::formatAddress(uint32_t address) const
{
    std::stringstream ss;
    
    // Different architectures commonly use different address formats
    switch (m_architecture) {
        case Architecture::M68K:
        case Architecture::ARM:
        case Architecture::MIPS:
            ss << "$" << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << address;
            break;
            
        case Architecture::Z80:
        case Architecture::M6502:
            ss << "$" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << (address & 0xFFFF);
            break;
            
        default:
            ss << "0x" << std::hex << std::setw(8) << std::setfill('0') << address;
            break;
    }
    
    return ss.str();
}

// Get the current disassembly architecture
DisassemblyViewer::Architecture DisassemblyViewer::getArchitecture() const
{
    return m_architecture;
}

// Get a string representation of the architecture
std::string DisassemblyViewer::architectureToString(Architecture arch)
{
    switch (arch) {
        case Architecture::M68K:  return "M68K";
        case Architecture::Z80:   return "Z80";
        case Architecture::M6502: return "M6502";
        case Architecture::MIPS:  return "MIPS";
        case Architecture::ARM:   return "ARM";
        case Architecture::UNKNOWN:
        default:
            return "UNKNOWN";
    }
}

// Initialize the viewer
bool DisassemblyViewer::initialize()
{
    if (!m_cpuCore) {
        std::cerr << "DisassemblyViewer: No CPU core provided" << std::endl;
        return false;
    }
    
    return true;
}

// Set the current address
void DisassemblyViewer::setCurrentAddress(uint32_t address)
{
    if (m_currentAddress != address) {
        m_currentAddress = address;
        
        if (m_addressChangedCallback) {
            m_addressChangedCallback(m_currentAddress);
        }
    }
}

// Get the current address
uint32_t DisassemblyViewer::getCurrentAddress() const
{
    return m_currentAddress;
}

// Disassemble a range of instructions
std::vector<CPUCore::DisassembledInstruction> DisassemblyViewer::disassembleRange(uint32_t startAddress, size_t count)
{
    std::vector<CPUCore::DisassembledInstruction> instructions;
    
    if (!m_cpuCore) {
        return instructions;
    }
    
    uint32_t address = startAddress;
    for (size_t i = 0; i < count; ++i) {
        auto instruction = m_cpuCore->disassembleInstruction(address);
        instructions.push_back(instruction);
        
        // Move to the next instruction
        address += instruction.size;
    }
    
    return instructions;
}

// Toggle a breakpoint
bool DisassemblyViewer::toggleBreakpoint(uint32_t address)
{
    bool result = false;
    
    auto it = m_breakpoints.find(address);
    if (it != m_breakpoints.end()) {
        // Remove the breakpoint
        m_breakpoints.erase(it);
        result = false;
    } else {
        // Add the breakpoint
        m_breakpoints[address] = true;
        result = true;
    }
    
    if (m_breakpointsChangedCallback) {
        m_breakpointsChangedCallback();
    }
    
    return result;
}

// Get all breakpoints
std::vector<uint32_t> DisassemblyViewer::getBreakpoints() const
{
    std::vector<uint32_t> breakpoints;
    
    for (const auto& pair : m_breakpoints) {
        breakpoints.push_back(pair.first);
    }
    
    return breakpoints;
}

// Step forward to the next instruction
void DisassemblyViewer::stepForward()
{
    if (!m_cpuCore) {
        return;
    }
    
    auto instruction = m_cpuCore->disassembleInstruction(m_currentAddress);
    setCurrentAddress(m_currentAddress + instruction.size);
}

// Step backward to the previous instruction
void DisassemblyViewer::stepBackward()
{
    if (!m_cpuCore) {
        return;
    }
    
    uint32_t prevAddress = findPreviousInstructionAddress(m_currentAddress);
    setCurrentAddress(prevAddress);
}

// Set the address changed callback
void DisassemblyViewer::setAddressChangedCallback(std::function<void(uint32_t)> callback)
{
    m_addressChangedCallback = callback;
}

// Set the breakpoints changed callback
void DisassemblyViewer::setBreakpointsChangedCallback(std::function<void()> callback)
{
    m_breakpointsChangedCallback = callback;
}

// Find the address of the previous instruction
uint32_t DisassemblyViewer::findPreviousInstructionAddress(uint32_t currentAddress)
{
    if (!m_cpuCore || currentAddress == 0) {
        return 0;
    }
    
    // Simple approach: try a few bytes before the current address
    // This is not perfect but works in most cases for fixed-width instructions
    // For variable-width instructions, this is more complex
    
    // Try up to 16 bytes before the current address
    for (int i = 1; i <= 16; ++i) {
        uint32_t testAddress = (currentAddress >= i) ? (currentAddress - i) : 0;
        auto instruction = m_cpuCore->disassembleInstruction(testAddress);
        
        if (testAddress + instruction.size == currentAddress) {
            return testAddress;
        }
    }
    
    // Fallback: just go back by the minimum instruction size (architecture dependent)
    // For most architectures, minimum is 1 or 2 bytes
    return (currentAddress > 2) ? (currentAddress - 2) : 0;
}

// Render the disassembly view
void DisassemblyViewer::render(float x, float y, float width, float height)
{
    if (!m_cpuCore) {
        return;
    }
    
    // Calculate how many instructions can fit in the view
    // This is a placeholder - actual implementation would depend on the UI framework
    const int linesPerView = 20;
    
    // Disassemble instructions starting from the current address
    auto instructions = disassembleRange(m_currentAddress, linesPerView);
    
    // Render each instruction
    // This is a placeholder - actual implementation would depend on the UI framework
    // In a real implementation, you would:
    // 1. Draw the background
    // 2. For each instruction:
    //    a. Draw the address
    //    b. Draw breakpoint indicator if present
    //    c. Draw the instruction bytes
    //    d. Draw the disassembled text
    //    e. Highlight the current instruction
    
    // Example pseudo-code for rendering:
    /*
    float lineHeight = 20.0f;
    float currentY = y;
    
    uint32_t address = m_currentAddress;
    for (const auto& instruction : instructions) {
        bool isCurrentLine = (address == m_currentAddress);
        bool hasBreakpoint = hasBreakpoint(address);
        
        // Draw background based on isCurrentLine
        if (isCurrentLine) {
            drawRect(x, currentY, width, lineHeight, highlightColor);
        }
        
        // Draw breakpoint indicator
        if (hasBreakpoint) {
            drawCircle(x + 10, currentY + lineHeight/2, 5, breakpointColor);
        }
        
        // Draw address
        drawText(x + 30, currentY, m_cpuCore->formatAddress(address), addressColor);
        
        // Draw bytes
        std::string bytesStr;
        for (uint8_t byte : instruction.bytes) {
            bytesStr += formatByte(byte) + " ";
        }
        drawText(x + 100, currentY, bytesStr, bytesColor);
        
        // Draw disassembly text
        drawText(x + 250, currentY, instruction.text, textColor);
        
        // Move to next line
        currentY += lineHeight;
        address += instruction.size;
    }
    */
} 