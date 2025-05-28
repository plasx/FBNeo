#include "cpu_core.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

// M68K instruction mnemonics
static const std::vector<std::string> M68K_REGISTERS = {
    "D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7",
    "A0", "A1", "A2", "A3", "A4", "A5", "A6", "A7/SP",
    "PC", "SR", "CCR", "USP", "ISP", "MSP"
};

// Factory method to create CPU cores
std::unique_ptr<CPUCore> CPUCore::createForArchitecture(DisassemblyViewer::Architecture architecture) {
    switch (architecture) {
        case DisassemblyViewer::Architecture::M68K:
            return std::make_unique<M68KCore>();
        case DisassemblyViewer::Architecture::Z80:
            return std::make_unique<Z80Core>();
        default:
            return nullptr;
    }
}

// M68KCore implementation
DisassemblyViewer::Architecture M68KCore::getArchitecture() const {
    return DisassemblyViewer::Architecture::M68K;
}

DisassemblyViewer::Instruction M68KCore::disassembleInstruction(
    uint32_t address,
    std::function<uint8_t(uint32_t)> memoryRead) const {
    
    DisassemblyViewer::Instruction instruction;
    instruction.address = address;
    
    // Read the opcode word
    uint16_t opcode = (memoryRead(address) << 8) | memoryRead(address + 1);
    
    // Simplified M68K disassembly - in a real implementation, this would be much more complex
    // This is just an example to show the structure
    switch ((opcode >> 12) & 0xF) {
        case 0x1: // MOVE.B
            {
                uint8_t srcReg = (opcode >> 0) & 0x7;
                uint8_t destReg = (opcode >> 9) & 0x7;
                instruction.mnemonic = "MOVE.B";
                instruction.operands = M68K_REGISTERS[srcReg] + "," + M68K_REGISTERS[destReg];
                instruction.size = 2;
                instruction.cycles = 4;
            }
            break;
        case 0x2: // MOVE.L
            {
                uint8_t srcReg = (opcode >> 0) & 0x7;
                uint8_t destReg = (opcode >> 9) & 0x7;
                instruction.mnemonic = "MOVE.L";
                instruction.operands = M68K_REGISTERS[srcReg] + "," + M68K_REGISTERS[destReg];
                instruction.size = 2;
                instruction.cycles = 4;
            }
            break;
        case 0x3: // MOVE.W
            {
                uint8_t srcReg = (opcode >> 0) & 0x7;
                uint8_t destReg = (opcode >> 9) & 0x7;
                instruction.mnemonic = "MOVE.W";
                instruction.operands = M68K_REGISTERS[srcReg] + "," + M68K_REGISTERS[destReg];
                instruction.size = 2;
                instruction.cycles = 4;
            }
            break;
        case 0x4: // Miscellaneous
            if ((opcode & 0xF1C0) == 0x4180) {
                instruction.mnemonic = "CHK";
                instruction.operands = M68K_REGISTERS[(opcode >> 0) & 0x7] + "," + M68K_REGISTERS[(opcode >> 9) & 0x7];
                instruction.size = 2;
                instruction.cycles = 10;
            } else if ((opcode & 0xFFC0) == 0x4E80) {
                instruction.mnemonic = "JSR";
                instruction.operands = "(" + M68K_REGISTERS[(opcode >> 0) & 0x7] + ")";
                instruction.size = 2;
                instruction.cycles = 16;
            } else if ((opcode & 0xFFC0) == 0x4E40) {
                instruction.mnemonic = "TRAP";
                instruction.operands = "#" + std::to_string(opcode & 0xF);
                instruction.size = 2;
                instruction.cycles = 34;
            } else if (opcode == 0x4E75) {
                instruction.mnemonic = "RTS";
                instruction.operands = "";
                instruction.size = 2;
                instruction.cycles = 16;
            } else if (opcode == 0x4E73) {
                instruction.mnemonic = "RTE";
                instruction.operands = "";
                instruction.size = 2;
                instruction.cycles = 20;
            } else {
                instruction.mnemonic = "???";
                instruction.operands = "";
                instruction.size = 2;
                instruction.cycles = 0;
            }
            break;
        case 0x6: // Bcc, BSR
            {
                uint8_t condition = (opcode >> 8) & 0xF;
                int8_t displacement = opcode & 0xFF;
                std::string condStr;
                
                switch (condition) {
                    case 0x0: condStr = "RA"; break;
                    case 0x1: condStr = "SR"; break;
                    case 0x2: condStr = "HI"; break;
                    case 0x3: condStr = "LS"; break;
                    case 0x4: condStr = "CC"; break;
                    case 0x5: condStr = "CS"; break;
                    case 0x6: condStr = "NE"; break;
                    case 0x7: condStr = "EQ"; break;
                    case 0x8: condStr = "VC"; break;
                    case 0x9: condStr = "VS"; break;
                    case 0xA: condStr = "PL"; break;
                    case 0xB: condStr = "MI"; break;
                    case 0xC: condStr = "GE"; break;
                    case 0xD: condStr = "LT"; break;
                    case 0xE: condStr = "GT"; break;
                    case 0xF: condStr = "LE"; break;
                }
                
                instruction.mnemonic = "B" + condStr;
                
                std::stringstream ss;
                ss << "$" << std::hex << std::uppercase << (address + 2 + displacement);
                instruction.operands = ss.str();
                instruction.size = 2;
                instruction.cycles = 10;
            }
            break;
        default:
            instruction.mnemonic = "???";
            instruction.operands = "";
            instruction.size = 2;
            instruction.cycles = 0;
            break;
    }
    
    instruction.hasBreakpoint = false;
    instruction.isProgramCounter = false;
    
    return instruction;
}

std::vector<DisassemblyViewer::Instruction> M68KCore::disassembleRange(
    uint32_t startAddress,
    size_t count,
    std::function<uint8_t(uint32_t)> memoryRead) const {
    
    std::vector<DisassemblyViewer::Instruction> instructions;
    instructions.reserve(count);
    
    uint32_t address = startAddress;
    for (size_t i = 0; i < count; i++) {
        auto instruction = disassembleInstruction(address, memoryRead);
        instructions.push_back(instruction);
        address += instruction.size;
    }
    
    return instructions;
}

uint8_t M68KCore::getInstructionSize(
    uint32_t address,
    std::function<uint8_t(uint32_t)> memoryRead) const {
    
    // In a real implementation, we would need to actually decode the instruction
    // Here we'll just use a simplified approach
    uint16_t opcode = (memoryRead(address) << 8) | memoryRead(address + 1);
    
    // Most common M68K instructions are 2, 4, or 6 bytes
    // For simplicity, we'll return a fixed size
    return 2;
}

bool M68KCore::isCallInstruction(const DisassemblyViewer::Instruction& instruction) const {
    return instruction.mnemonic == "JSR" || instruction.mnemonic == "BSR";
}

bool M68KCore::isReturnInstruction(const DisassemblyViewer::Instruction& instruction) const {
    return instruction.mnemonic == "RTS" || instruction.mnemonic == "RTE" || instruction.mnemonic == "RTR";
}

bool M68KCore::isJumpInstruction(const DisassemblyViewer::Instruction& instruction) const {
    return instruction.mnemonic == "JMP" ||
           instruction.mnemonic.substr(0, 1) == "B"; // Any branch instruction
}

uint32_t M68KCore::getBranchTargetAddress(
    const DisassemblyViewer::Instruction& instruction,
    uint32_t currentAddress) const {
    
    // Parse the operand string to get the target address
    // This is a simplified implementation
    if (instruction.mnemonic.substr(0, 1) == "B" && instruction.operands.substr(0, 1) == "$") {
        std::string addrHex = instruction.operands.substr(1);
        try {
            return std::stoul(addrHex, nullptr, 16);
        } catch (...) {
            return 0;
        }
    }
    
    return 0;
}

std::vector<std::string> M68KCore::getRegisterNames() const {
    return M68K_REGISTERS;
}

// Z80Core implementation - basic implementation for now
DisassemblyViewer::Architecture Z80Core::getArchitecture() const {
    return DisassemblyViewer::Architecture::Z80;
}

DisassemblyViewer::Instruction Z80Core::disassembleInstruction(
    uint32_t address,
    std::function<uint8_t(uint32_t)> memoryRead) const {
    
    DisassemblyViewer::Instruction instruction;
    instruction.address = address;
    
    // Simple Z80 disassembly with just a few opcodes for demonstration
    uint8_t opcode = memoryRead(address);
    
    switch (opcode) {
        case 0x00:
            instruction.mnemonic = "NOP";
            instruction.operands = "";
            instruction.size = 1;
            instruction.cycles = 4;
            break;
        case 0x01:
            instruction.mnemonic = "LD";
            instruction.operands = "BC,";
            instruction.operands += "#$" + 
                std::to_string(memoryRead(address + 2) * 256 + memoryRead(address + 1));
            instruction.size = 3;
            instruction.cycles = 10;
            break;
        case 0xC3:
            instruction.mnemonic = "JP";
            instruction.operands = "$" + 
                std::to_string(memoryRead(address + 2) * 256 + memoryRead(address + 1));
            instruction.size = 3;
            instruction.cycles = 10;
            break;
        case 0xC9:
            instruction.mnemonic = "RET";
            instruction.operands = "";
            instruction.size = 1;
            instruction.cycles = 10;
            break;
        case 0xCD:
            instruction.mnemonic = "CALL";
            instruction.operands = "$" + 
                std::to_string(memoryRead(address + 2) * 256 + memoryRead(address + 1));
            instruction.size = 3;
            instruction.cycles = 17;
            break;
        default:
            instruction.mnemonic = "???";
            instruction.operands = "";
            instruction.size = 1;
            instruction.cycles = 0;
            break;
    }
    
    instruction.hasBreakpoint = false;
    instruction.isProgramCounter = false;
    
    return instruction;
}

std::vector<DisassemblyViewer::Instruction> Z80Core::disassembleRange(
    uint32_t startAddress,
    size_t count,
    std::function<uint8_t(uint32_t)> memoryRead) const {
    
    std::vector<DisassemblyViewer::Instruction> instructions;
    instructions.reserve(count);
    
    uint32_t address = startAddress;
    for (size_t i = 0; i < count; i++) {
        auto instruction = disassembleInstruction(address, memoryRead);
        instructions.push_back(instruction);
        address += instruction.size;
    }
    
    return instructions;
}

uint8_t Z80Core::getInstructionSize(
    uint32_t address,
    std::function<uint8_t(uint32_t)> memoryRead) const {
    
    // Basic implementation, in reality this would need to decode the instruction completely
    uint8_t opcode = memoryRead(address);
    switch (opcode) {
        case 0x01: // LD BC,nn
        case 0xC3: // JP nn
        case 0xCD: // CALL nn
            return 3;
        default:
            return 1;
    }
}

bool Z80Core::isCallInstruction(const DisassemblyViewer::Instruction& instruction) const {
    return instruction.mnemonic == "CALL";
}

bool Z80Core::isReturnInstruction(const DisassemblyViewer::Instruction& instruction) const {
    return instruction.mnemonic == "RET";
}

bool Z80Core::isJumpInstruction(const DisassemblyViewer::Instruction& instruction) const {
    return instruction.mnemonic == "JP" || instruction.mnemonic == "JR";
}

uint32_t Z80Core::getBranchTargetAddress(
    const DisassemblyViewer::Instruction& instruction,
    uint32_t currentAddress) const {
    
    // Parse the operand string to get the target address
    if ((instruction.mnemonic == "JP" || instruction.mnemonic == "CALL") &&
        instruction.operands.substr(0, 1) == "$") {
        try {
            return std::stoul(instruction.operands.substr(1), nullptr, 10);
        } catch (...) {
            return 0;
        }
    }
    
    return 0;
}

std::vector<std::string> Z80Core::getRegisterNames() const {
    return {
        "A", "F", "B", "C", "D", "E", "H", "L",
        "IX", "IY", "SP", "PC", "I", "R"
    };
} 