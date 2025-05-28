#include "register_viewer.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <bitset>

// Constructor
RegisterViewer::RegisterViewer()
    : m_architecture("")
    , m_defaultDisplayFormat(DisplayFormat::Hex)
    , m_registerUpdateCallback(nullptr)
    , m_editMode(false)
    , m_editingRegister("")
    , m_editBuffer("")
{
}

// Destructor
RegisterViewer::~RegisterViewer()
{
}

// Initialize the register viewer
bool RegisterViewer::initialize()
{
    // Initialize with default settings
    m_defaultDisplayFormat = DisplayFormat::Hex;
    m_registers.clear();
    m_groups.clear();
    m_registerDisplayFormats.clear();
    return true;
}

// Set the CPU architecture
bool RegisterViewer::setArchitecture(const std::string& architecture)
{
    // Store previous architecture
    std::string prevArchitecture = m_architecture;
    
    // Check if architecture is supported
    if (architecture != "M68K" && architecture != "Z80" && 
        architecture != "ARM" && architecture != "MIPS" && 
        architecture != "M6502") {
        return false;
    }
    
    // Set new architecture
    m_architecture = architecture;
    
    // Clear existing registers
    m_registers.clear();
    m_groups.clear();
    
    // Initialize registers for the new architecture
    initializeArchitectureRegisters(architecture);
    
    return true;
}

// Get the current CPU architecture
std::string RegisterViewer::getArchitecture() const
{
    return m_architecture;
}

// Update register values
void RegisterViewer::updateRegisters(const std::unordered_map<std::string, uint64_t>& registers)
{
    for (const auto& reg : registers) {
        auto it = m_registers.find(reg.first);
        if (it != m_registers.end()) {
            it->second.prevValue = it->second.value;
            it->second.value = reg.second;
        }
    }
}

// Set a specific register value
bool RegisterViewer::setRegisterValue(const std::string& name, uint64_t value)
{
    auto it = m_registers.find(name);
    if (it == m_registers.end() || it->second.isReadOnly) {
        return false;
    }
    
    it->second.prevValue = it->second.value;
    it->second.value = value;
    
    // Call update callback if set
    if (m_registerUpdateCallback) {
        m_registerUpdateCallback(name, value);
    }
    
    return true;
}

// Get a specific register value
bool RegisterViewer::getRegisterValue(const std::string& name, uint64_t& value) const
{
    auto it = m_registers.find(name);
    if (it == m_registers.end()) {
        return false;
    }
    
    value = it->second.value;
    return true;
}

// Check if a register has changed since the last update
bool RegisterViewer::hasRegisterChanged(const std::string& name) const
{
    auto it = m_registers.find(name);
    if (it == m_registers.end()) {
        return false;
    }
    
    return it->second.value != it->second.prevValue;
}

// Set register update callback
void RegisterViewer::setRegisterUpdateCallback(std::function<void(const std::string&, uint64_t)> callback)
{
    m_registerUpdateCallback = callback;
}

// Set display format for all registers
void RegisterViewer::setDisplayFormat(DisplayFormat format)
{
    m_defaultDisplayFormat = format;
}

// Set display format for a specific register
void RegisterViewer::setRegisterDisplayFormat(const std::string& registerName, DisplayFormat format)
{
    m_registerDisplayFormats[registerName] = format;
}

// Update the register viewer
void RegisterViewer::update(float deltaTime)
{
    if (m_editMode) {
        handleInput();
    }
}

// Render the register viewer
void RegisterViewer::render(float x, float y, float width, float height)
{
    // This would typically use ImGui or another UI system
    // For this implementation, we'll just log a message
    // In the actual implementation, you would render each register group
    // and its registers in a scrollable UI
    
    std::cout << "Rendering RegisterViewer for " << m_architecture << " at (" 
              << x << ", " << y << ") with size " << width << "x" << height << std::endl;
              
    // Display all register groups
    for (const auto& group : m_groups) {
        std::cout << "Register Group: " << group.name << std::endl;
        
        for (const auto& regName : group.registers) {
            const auto& reg = m_registers[regName];
            DisplayFormat format = m_registerDisplayFormats.count(regName) > 0 ? 
                                   m_registerDisplayFormats[regName] : m_defaultDisplayFormat;
                                   
            std::string valueStr = formatRegisterValue(reg.value, reg.size, format);
            std::string changedIndicator = hasRegisterChanged(regName) ? " *" : "  ";
            
            std::cout << "  " << std::setw(10) << regName << changedIndicator 
                      << valueStr << " (" << reg.size << " bits)" 
                      << (reg.isReadOnly ? " [RO]" : "") << std::endl;
        }
    }
}

// Define a register group
void RegisterViewer::defineRegisterGroup(const std::string& name, const std::vector<std::string>& registers)
{
    // Check if group already exists
    auto it = std::find_if(m_groups.begin(), m_groups.end(), 
                          [&name](const RegisterGroup& group) {
                              return group.name == name;
                          });
    
    if (it != m_groups.end()) {
        // Update existing group
        it->registers = registers;
    } else {
        // Create new group
        RegisterGroup group;
        group.name = name;
        group.registers = registers;
        m_groups.push_back(group);
    }
}

// Define register information
void RegisterViewer::defineRegister(const std::string& name, uint32_t size, const std::string& group,
                                  const std::string& description, bool isReadOnly)
{
    RegisterInfo info;
    info.name = name;
    info.value = 0;
    info.prevValue = 0;
    info.size = size;
    info.group = group;
    info.description = description;
    info.isReadOnly = isReadOnly;
    
    m_registers[name] = info;
}

// Clear all register definitions
void RegisterViewer::clearRegisters()
{
    m_registers.clear();
    m_groups.clear();
}

// Get all register groups
std::vector<RegisterViewer::RegisterGroup> RegisterViewer::getRegisterGroups() const
{
    return m_groups;
}

// Get all registers in a group
std::vector<RegisterViewer::RegisterInfo> RegisterViewer::getRegistersInGroup(const std::string& groupName) const
{
    std::vector<RegisterInfo> result;
    
    auto it = std::find_if(m_groups.begin(), m_groups.end(),
                          [&groupName](const RegisterGroup& group) {
                              return group.name == groupName;
                          });
    
    if (it != m_groups.end()) {
        for (const auto& regName : it->registers) {
            auto regIt = m_registers.find(regName);
            if (regIt != m_registers.end()) {
                result.push_back(regIt->second);
            }
        }
    }
    
    return result;
}

// Get all register infos
std::unordered_map<std::string, RegisterViewer::RegisterInfo> RegisterViewer::getAllRegisters() const
{
    return m_registers;
}

// Format a register value as a string
std::string RegisterViewer::formatRegisterValue(uint64_t value, uint32_t size, DisplayFormat format) const
{
    std::stringstream ss;
    
    switch (format) {
        case DisplayFormat::Hex:
            ss << "0x" << std::hex << std::uppercase << std::setfill('0');
            if (size <= 8) {
                ss << std::setw(2) << (value & 0xFF);
            } else if (size <= 16) {
                ss << std::setw(4) << (value & 0xFFFF);
            } else if (size <= 32) {
                ss << std::setw(8) << (value & 0xFFFFFFFF);
            } else {
                ss << std::setw(16) << value;
            }
            break;
            
        case DisplayFormat::Decimal:
            ss << std::dec << value;
            break;
            
        case DisplayFormat::Binary:
            if (size <= 8) {
                ss << std::bitset<8>(value).to_string();
            } else if (size <= 16) {
                ss << std::bitset<16>(value).to_string();
            } else if (size <= 32) {
                ss << std::bitset<32>(value).to_string();
            } else {
                ss << std::bitset<64>(value).to_string();
            }
            break;
            
        case DisplayFormat::ASCII:
            if (size <= 8) {
                char c = static_cast<char>(value & 0xFF);
                if (c >= 32 && c <= 126) { // Printable ASCII
                    ss << "'" << c << "'";
                } else {
                    ss << "\\x" << std::hex << std::uppercase << std::setfill('0')
                       << std::setw(2) << (value & 0xFF);
                }
            } else {
                ss << "N/A";
            }
            break;
    }
    
    return ss.str();
}

// Initialize registers for a specific architecture
void RegisterViewer::initializeArchitectureRegisters(const std::string& architecture)
{
    if (architecture == "M68K") {
        initializeM68KRegisters();
    } else if (architecture == "Z80") {
        initializeZ80Registers();
    } else if (architecture == "ARM") {
        initializeARMRegisters();
    } else if (architecture == "MIPS") {
        initializeMIPSRegisters();
    } else if (architecture == "M6502") {
        initializeM6502Registers();
    }
}

// Initialize M68K registers
void RegisterViewer::initializeM68KRegisters()
{
    // Define M68K data registers
    std::vector<std::string> dataRegs = {"D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7"};
    defineRegisterGroup("Data Registers", dataRegs);
    
    for (const auto& reg : dataRegs) {
        defineRegister(reg, 32, "Data Registers", "32-bit data register", false);
    }
    
    // Define M68K address registers
    std::vector<std::string> addrRegs = {"A0", "A1", "A2", "A3", "A4", "A5", "A6", "A7"};
    defineRegisterGroup("Address Registers", addrRegs);
    
    for (const auto& reg : addrRegs) {
        defineRegister(reg, 32, "Address Registers", "32-bit address register", false);
    }
    
    // Define M68K special registers
    std::vector<std::string> specialRegs = {"PC", "SR", "USP", "SSP"};
    defineRegisterGroup("Special Registers", specialRegs);
    
    defineRegister("PC", 32, "Special Registers", "Program Counter", false);
    defineRegister("SR", 16, "Special Registers", "Status Register", false);
    defineRegister("USP", 32, "Special Registers", "User Stack Pointer", false);
    defineRegister("SSP", 32, "Special Registers", "Supervisor Stack Pointer", false);
}

// Initialize Z80 registers
void RegisterViewer::initializeZ80Registers()
{
    // Main register pairs
    std::vector<std::string> mainRegs = {"AF", "BC", "DE", "HL"};
    defineRegisterGroup("Main Registers", mainRegs);
    
    for (const auto& reg : mainRegs) {
        defineRegister(reg, 16, "Main Registers", reg[0] + std::string(" and ") + reg[1] + " register pair", false);
    }
    
    // Individual 8-bit registers
    std::vector<std::string> byteRegs = {"A", "F", "B", "C", "D", "E", "H", "L"};
    defineRegisterGroup("8-bit Registers", byteRegs);
    
    defineRegister("A", 8, "8-bit Registers", "Accumulator", false);
    defineRegister("F", 8, "8-bit Registers", "Flags", false);
    defineRegister("B", 8, "8-bit Registers", "B register", false);
    defineRegister("C", 8, "8-bit Registers", "C register", false);
    defineRegister("D", 8, "8-bit Registers", "D register", false);
    defineRegister("E", 8, "8-bit Registers", "E register", false);
    defineRegister("H", 8, "8-bit Registers", "H register", false);
    defineRegister("L", 8, "8-bit Registers", "L register", false);
    
    // Alternate registers
    std::vector<std::string> altRegs = {"AF'", "BC'", "DE'", "HL'"};
    defineRegisterGroup("Alternate Registers", altRegs);
    
    for (const auto& reg : altRegs) {
        defineRegister(reg, 16, "Alternate Registers", "Alternate " + reg.substr(0, 2) + " register pair", false);
    }
    
    // Index and special registers
    std::vector<std::string> specialRegs = {"IX", "IY", "SP", "PC", "I", "R"};
    defineRegisterGroup("Special Registers", specialRegs);
    
    defineRegister("IX", 16, "Special Registers", "Index Register X", false);
    defineRegister("IY", 16, "Special Registers", "Index Register Y", false);
    defineRegister("SP", 16, "Special Registers", "Stack Pointer", false);
    defineRegister("PC", 16, "Special Registers", "Program Counter", false);
    defineRegister("I", 8, "Special Registers", "Interrupt Vector", false);
    defineRegister("R", 8, "Special Registers", "Memory Refresh", false);
}

// Initialize ARM registers
void RegisterViewer::initializeARMRegisters()
{
    // General purpose registers
    std::vector<std::string> gpRegs;
    for (int i = 0; i < 16; i++) {
        gpRegs.push_back("R" + std::to_string(i));
    }
    defineRegisterGroup("General Purpose", gpRegs);
    
    for (int i = 0; i < 16; i++) {
        std::string reg = "R" + std::to_string(i);
        std::string desc = "General Purpose Register";
        
        if (i == 13) desc = "Stack Pointer (SP)";
        else if (i == 14) desc = "Link Register (LR)";
        else if (i == 15) desc = "Program Counter (PC)";
        
        defineRegister(reg, 32, "General Purpose", desc, false);
    }
    
    // Status registers
    std::vector<std::string> statusRegs = {"CPSR", "SPSR"};
    defineRegisterGroup("Status Registers", statusRegs);
    
    defineRegister("CPSR", 32, "Status Registers", "Current Program Status Register", false);
    defineRegister("SPSR", 32, "Status Registers", "Saved Program Status Register", false);
}

// Initialize MIPS registers
void RegisterViewer::initializeMIPSRegisters()
{
    // General purpose registers
    std::vector<std::string> gpRegs;
    for (int i = 0; i < 32; i++) {
        gpRegs.push_back("$" + std::to_string(i));
    }
    defineRegisterGroup("General Purpose", gpRegs);
    
    std::vector<std::string> aliases = {
        "zero", "at", "v0", "v1", "a0", "a1", "a2", "a3",
        "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
        "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
        "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra"
    };
    
    for (int i = 0; i < 32; i++) {
        std::string desc = "General Purpose Register (" + aliases[i] + ")";
        defineRegister("$" + std::to_string(i), 32, "General Purpose", desc, i == 0);
    }
    
    // Special registers
    std::vector<std::string> specialRegs = {"PC", "HI", "LO"};
    defineRegisterGroup("Special Registers", specialRegs);
    
    defineRegister("PC", 32, "Special Registers", "Program Counter", false);
    defineRegister("HI", 32, "Special Registers", "Multiply/Divide Result High", false);
    defineRegister("LO", 32, "Special Registers", "Multiply/Divide Result Low", false);
}

// Initialize M6502 registers
void RegisterViewer::initializeM6502Registers()
{
    // Main registers
    std::vector<std::string> mainRegs = {"A", "X", "Y", "S", "P", "PC"};
    defineRegisterGroup("Registers", mainRegs);
    
    defineRegister("A", 8, "Registers", "Accumulator", false);
    defineRegister("X", 8, "Registers", "X Index Register", false);
    defineRegister("Y", 8, "Registers", "Y Index Register", false);
    defineRegister("S", 8, "Registers", "Stack Pointer", false);
    defineRegister("P", 8, "Registers", "Processor Status", false);
    defineRegister("PC", 16, "Registers", "Program Counter", false);
    
    // Individual status flags
    std::vector<std::string> flags = {"N", "V", "B", "D", "I", "Z", "C"};
    defineRegisterGroup("Status Flags", flags);
    
    defineRegister("N", 1, "Status Flags", "Negative Flag", false);
    defineRegister("V", 1, "Status Flags", "Overflow Flag", false);
    defineRegister("B", 1, "Status Flags", "Break Command", false);
    defineRegister("D", 1, "Status Flags", "Decimal Mode", false);
    defineRegister("I", 1, "Status Flags", "Interrupt Disable", false);
    defineRegister("Z", 1, "Status Flags", "Zero Flag", false);
    defineRegister("C", 1, "Status Flags", "Carry Flag", false);
}

// Handle input for register editing
void RegisterViewer::handleInput()
{
    // In a real implementation, this would handle keyboard input
    // for register editing
    
    if (!m_editMode || m_editingRegister.empty()) {
        return;
    }
    
    auto it = m_registers.find(m_editingRegister);
    if (it == m_registers.end() || it->second.isReadOnly) {
        m_editMode = false;
        m_editingRegister = "";
        m_editBuffer = "";
        return;
    }
    
    // Parse the edit buffer when editing is confirmed
    uint64_t value;
    if (parseRegisterValue(m_editBuffer, value, it->second.size)) {
        setRegisterValue(m_editingRegister, value);
    }
    
    // Exit edit mode
    m_editMode = false;
    m_editingRegister = "";
    m_editBuffer = "";
}

// Parse a register value from a string
bool RegisterViewer::parseRegisterValue(const std::string& input, uint64_t& value, uint32_t size) const
{
    try {
        // Try to parse as hex if starts with "0x"
        if (input.substr(0, 2) == "0x" || input.substr(0, 2) == "0X") {
            value = std::stoull(input.substr(2), nullptr, 16);
        }
        // Try to parse as binary if starts with "0b"
        else if (input.substr(0, 2) == "0b" || input.substr(0, 2) == "0B") {
            value = std::stoull(input.substr(2), nullptr, 2);
        }
        // Otherwise parse as decimal
        else {
            value = std::stoull(input);
        }
        
        // Mask to appropriate size
        uint64_t mask = 0;
        if (size == 8) mask = 0xFF;
        else if (size == 16) mask = 0xFFFF;
        else if (size == 32) mask = 0xFFFFFFFF;
        else mask = 0xFFFFFFFFFFFFFFFF;
        
        value &= mask;
        return true;
    } catch (const std::exception&) {
        return false;
    }
} 