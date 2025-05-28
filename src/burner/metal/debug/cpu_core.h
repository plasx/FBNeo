#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <functional>
#include <memory>

/**
 * @class CPUCore
 * @brief Abstract base class for CPU-specific disassembly and analysis
 * 
 * This class provides a common interface for architecture-specific disassembly,
 * register information, and instruction analysis. Implementations are provided
 * for various CPU architectures like M68K, Z80, etc.
 */
class CPUCore {
public:
    /**
     * @brief Virtual destructor
     */
    virtual ~CPUCore() = default;
    
    /**
     * @struct DisassembledInstruction
     * @brief Structure representing a disassembled instruction
     */
    struct DisassembledInstruction {
        std::string mnemonic;       ///< Instruction mnemonic (e.g., "MOV", "ADD")
        std::string operands;       ///< Instruction operands (e.g., "D0, D1")
        uint8_t size;               ///< Size of the instruction in bytes
        std::vector<uint8_t> bytes; ///< Raw bytes of the instruction
    };
    
    /**
     * @brief Disassemble a single instruction at the specified address
     * @param address The memory address to disassemble
     * @return Disassembled instruction information
     */
    virtual DisassembledInstruction disassemble(uint32_t address) const = 0;
    
    /**
     * @brief Get the list of register names for this CPU architecture
     * @return Vector of register names
     */
    virtual std::vector<std::string> getRegisterNames() const = 0;
    
    /**
     * @brief Set the memory read function for accessing ROM/RAM
     * @param readFn Function that takes an address and returns a byte
     */
    virtual void setMemoryReadFunction(std::function<uint8_t(uint32_t)> readFn) = 0;
    
    /**
     * @brief Check if the instruction at the given address is a call instruction
     * @param address The address to check
     * @return True if the instruction is a call, false otherwise
     */
    virtual bool isCallInstruction(uint32_t address) const = 0;
    
    /**
     * @brief Check if the instruction at the given address is a return instruction
     * @param address The address to check
     * @return True if the instruction is a return, false otherwise
     */
    virtual bool isReturnInstruction(uint32_t address) const = 0;
    
    /**
     * @brief Check if the instruction at the given address is a jump instruction
     * @param address The address to check
     * @return True if the instruction is a jump, false otherwise
     */
    virtual bool isJumpInstruction(uint32_t address) const = 0;
    
    /**
     * @brief Get the target address for a branch/call/jump instruction
     * @param address The address of the branch instruction
     * @return The target address, or 0 if not applicable
     */
    virtual uint32_t getTargetAddress(uint32_t address) const = 0;
    
    /**
     * @brief Format an address according to this architecture's convention
     * @param address The address to format
     * @return Formatted address string
     */
    virtual std::string formatAddress(uint32_t address) const = 0;
    
    /**
     * @brief Factory method to create a CPU core for the specified architecture
     * @param architecture The name of the architecture (e.g., "M68K", "Z80")
     * @return Unique pointer to the created CPU core, or nullptr if unsupported
     */
    static std::unique_ptr<CPUCore> createForArchitecture(const std::string& architecture);
};

/**
 * @class M68KCore
 * @brief Disassembler for Motorola 68000 series CPUs
 */
class M68KCore : public CPUCore {
public:
    DisassembledInstruction disassemble(uint32_t address) const override;
    std::vector<std::string> getRegisterNames() const override;
    void setMemoryReadFunction(std::function<uint8_t(uint32_t)> memoryRead) override;
    bool isCallInstruction(uint32_t address) const override;
    bool isReturnInstruction(uint32_t address) const override;
    bool isJumpInstruction(uint32_t address) const override;
    uint32_t getTargetAddress(uint32_t address) const override;
    std::string formatAddress(uint32_t address) const override;
};

/**
 * @class Z80Core
 * @brief Disassembler for Zilog Z80 CPUs
 */
class Z80Core : public CPUCore {
public:
    DisassembledInstruction disassemble(uint32_t address) const override;
    std::vector<std::string> getRegisterNames() const override;
    void setMemoryReadFunction(std::function<uint8_t(uint32_t)> memoryRead) override;
    bool isCallInstruction(uint32_t address) const override;
    bool isReturnInstruction(uint32_t address) const override;
    bool isJumpInstruction(uint32_t address) const override;
    uint32_t getTargetAddress(uint32_t address) const override;
    std::string formatAddress(uint32_t address) const override;
}; 