#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <unordered_map>
#include <cstdint>

/**
 * @class RegisterViewer
 * @brief Provides visualization and editing of CPU registers
 * 
 * This class renders and allows editing of CPU registers for various architectures.
 * It supports different register groups (general purpose, control, etc.) and
 * can display registers in various formats (hex, decimal, binary).
 */
class RegisterViewer {
public:
    /**
     * @enum DisplayFormat
     * @brief Format for displaying register values
     */
    enum class DisplayFormat {
        Hex,        ///< Hexadecimal display (default)
        Decimal,    ///< Decimal display
        Binary,     ///< Binary display
        ASCII       ///< ASCII display (for byte registers)
    };
    
    /**
     * @struct RegisterGroup
     * @brief Group of related registers
     */
    struct RegisterGroup {
        std::string name;               ///< Group name (e.g., "General Purpose", "Control")
        std::vector<std::string> registers; ///< Register names in this group
    };
    
    /**
     * @struct RegisterInfo
     * @brief Information about a specific register
     */
    struct RegisterInfo {
        std::string name;       ///< Register name (e.g., "D0", "A0", "PC")
        uint64_t value;         ///< Current value of the register
        uint64_t prevValue;     ///< Previous value for change highlighting
        uint32_t size;          ///< Size in bits (8, 16, 32, or 64)
        std::string group;      ///< Group this register belongs to
        std::string description; ///< Optional description
        bool isReadOnly;        ///< Whether this register is read-only
    };
    
    /**
     * @brief Constructor
     */
    RegisterViewer();
    
    /**
     * @brief Destructor
     */
    ~RegisterViewer();
    
    /**
     * @brief Initialize the register viewer
     * @return True if initialization was successful, false otherwise
     */
    bool initialize();
    
    /**
     * @brief Set the CPU architecture
     * @param architecture The CPU architecture (e.g., "M68K", "Z80")
     * @return True if the architecture is supported, false otherwise
     */
    bool setArchitecture(const std::string& architecture);
    
    /**
     * @brief Get the current CPU architecture
     * @return The current CPU architecture
     */
    std::string getArchitecture() const;
    
    /**
     * @brief Update register values
     * @param registers Map of register names to values
     */
    void updateRegisters(const std::unordered_map<std::string, uint64_t>& registers);
    
    /**
     * @brief Set a specific register value
     * @param name Register name
     * @param value New register value
     * @return True if register was found and updated, false otherwise
     */
    bool setRegisterValue(const std::string& name, uint64_t value);
    
    /**
     * @brief Get a specific register value
     * @param name Register name
     * @param value Output parameter for register value
     * @return True if register was found, false otherwise
     */
    bool getRegisterValue(const std::string& name, uint64_t& value) const;
    
    /**
     * @brief Check if a register has changed since the last update
     * @param name Register name
     * @return True if register has changed, false otherwise
     */
    bool hasRegisterChanged(const std::string& name) const;
    
    /**
     * @brief Set register update callback
     * @param callback Function to call when registers are changed
     */
    void setRegisterUpdateCallback(std::function<void(const std::string&, uint64_t)> callback);
    
    /**
     * @brief Set display format for all registers
     * @param format The display format
     */
    void setDisplayFormat(DisplayFormat format);
    
    /**
     * @brief Set display format for a specific register
     * @param registerName Register name
     * @param format The display format
     */
    void setRegisterDisplayFormat(const std::string& registerName, DisplayFormat format);
    
    /**
     * @brief Update the register viewer
     * @param deltaTime Time elapsed since last update
     */
    void update(float deltaTime);
    
    /**
     * @brief Render the register viewer
     * @param x X-coordinate for rendering
     * @param y Y-coordinate for rendering
     * @param width Width of the rendering area
     * @param height Height of the rendering area
     */
    void render(float x, float y, float width, float height);
    
    /**
     * @brief Define a register group
     * @param name Group name
     * @param registers Register names in this group
     */
    void defineRegisterGroup(const std::string& name, const std::vector<std::string>& registers);
    
    /**
     * @brief Define register information
     * @param name Register name
     * @param size Register size in bits
     * @param group Register group
     * @param description Optional description
     * @param isReadOnly Whether register is read-only
     */
    void defineRegister(const std::string& name, uint32_t size, const std::string& group,
                       const std::string& description = "", bool isReadOnly = false);
    
    /**
     * @brief Clear all register definitions
     */
    void clearRegisters();
    
    /**
     * @brief Get all register groups
     * @return Vector of register groups
     */
    std::vector<RegisterGroup> getRegisterGroups() const;
    
    /**
     * @brief Get all registers in a group
     * @param groupName Group name
     * @return Vector of register infos
     */
    std::vector<RegisterInfo> getRegistersInGroup(const std::string& groupName) const;
    
    /**
     * @brief Get all register infos
     * @return Map of register names to register infos
     */
    std::unordered_map<std::string, RegisterInfo> getAllRegisters() const;
    
    /**
     * @brief Format a register value as a string
     * @param value Register value
     * @param size Register size in bits
     * @param format Display format
     * @return Formatted string
     */
    std::string formatRegisterValue(uint64_t value, uint32_t size, DisplayFormat format) const;

private:
    std::string m_architecture;
    std::unordered_map<std::string, RegisterInfo> m_registers;
    std::vector<RegisterGroup> m_groups;
    DisplayFormat m_defaultDisplayFormat;
    std::unordered_map<std::string, DisplayFormat> m_registerDisplayFormats;
    std::function<void(const std::string&, uint64_t)> m_registerUpdateCallback;
    
    bool m_editMode;
    std::string m_editingRegister;
    std::string m_editBuffer;
    
    void initializeArchitectureRegisters(const std::string& architecture);
    void initializeM68KRegisters();
    void initializeZ80Registers();
    void initializeARMRegisters();
    void initializeMIPSRegisters();
    void initializeM6502Registers();
    
    void handleInput();
    bool parseRegisterValue(const std::string& input, uint64_t& value, uint32_t size) const;
}; 