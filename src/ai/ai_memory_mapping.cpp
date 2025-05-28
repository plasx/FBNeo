#include "ai_memory_mapping.h"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <cstring>
#include <algorithm>
#include <cmath>

// For Windows compatibility
#if defined(_WIN32)
#include <windows.h>
#define PATH_SEPARATOR "\\"
#else
#define PATH_SEPARATOR "/"
#endif

// For memory access
extern unsigned char* CpuRam;  // M68K RAM
extern unsigned char* MainRam; // Main RAM
extern unsigned char* Z80Ram;  // Z80 RAM
extern unsigned char* CpsRam;  // CPS RAM
extern unsigned char* CpsFrg;  // CPS Foreground RAM
extern unsigned char* CpsZRam; // CPS Z80 RAM

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace AI {

// External memory access function signature
// This would be provided by the emulator core
extern "C" {
    uint8_t ReadMemory(uint32_t address);
}

// Helper function to convert a hex string to a uint32_t
static uint32_t HexToUint32(const std::string& hex) {
    // Check if the string starts with "0x"
    if (hex.size() > 2 && hex.substr(0, 2) == "0x") {
        return static_cast<uint32_t>(std::stoul(hex.substr(2), nullptr, 16));
    }
    return static_cast<uint32_t>(std::stoul(hex, nullptr, 16));
}

// Helper function to convert a value variant to double
static double ValueTypeToDouble(const AIMemoryMapping::ValueType& value) {
    if (std::holds_alternative<uint8_t>(value)) {
        return static_cast<double>(std::get<uint8_t>(value));
    } else if (std::holds_alternative<uint16_t>(value)) {
        return static_cast<double>(std::get<uint16_t>(value));
    } else if (std::holds_alternative<uint32_t>(value)) {
        return static_cast<double>(std::get<uint32_t>(value));
    } else if (std::holds_alternative<float>(value)) {
        return static_cast<double>(std::get<float>(value));
    } else if (std::holds_alternative<bool>(value)) {
        return std::get<bool>(value) ? 1.0 : 0.0;
    }
    return 0.0;
}

// MemoryMappingEntry implementation
MemoryMappingEntry::MemoryMappingEntry(const std::string& name, unsigned int address, 
                                      unsigned int size, MemoryType type, 
                                      Endianness endianness, bool isArray)
    : m_name(name)
    , m_address(address)
    , m_size(size)
    , m_type(type)
    , m_endianness(endianness)
    , m_isArray(isArray)
    , m_value()
    , m_previousValue()
    , m_hasChanged(false)
{
}

std::string MemoryMappingEntry::getName() const
{
    return m_name;
}

unsigned int MemoryMappingEntry::getAddress() const
{
    return m_address;
}

unsigned int MemoryMappingEntry::getSize() const
{
    return m_size;
}

MemoryType MemoryMappingEntry::getType() const
{
    return m_type;
}

Endianness MemoryMappingEntry::getEndianness() const
{
    return m_endianness;
}

bool MemoryMappingEntry::isArray() const
{
    return m_isArray;
}

std::variant<int, float, std::vector<uint8_t>> MemoryMappingEntry::getValue() const
{
    return m_value;
}

std::string MemoryMappingEntry::getValueAsString() const
{
    std::stringstream ss;
    
    if (std::holds_alternative<int>(m_value)) {
        ss << std::get<int>(m_value);
    } else if (std::holds_alternative<float>(m_value)) {
        ss << std::get<float>(m_value);
    } else if (std::holds_alternative<std::vector<uint8_t>>(m_value)) {
        const auto& bytes = std::get<std::vector<uint8_t>>(m_value);
        ss << "[";
        for (size_t i = 0; i < bytes.size(); ++i) {
            ss << static_cast<int>(bytes[i]);
            if (i < bytes.size() - 1) {
                ss << ", ";
            }
        }
        ss << "]";
    }
    
    return ss.str();
}

bool MemoryMappingEntry::hasChanged() const
{
    return m_hasChanged;
}

void MemoryMappingEntry::setValue(const std::variant<int, float, std::vector<uint8_t>>& value)
{
    m_previousValue = m_value;
    m_value = value;
    
    // Check if the value has changed
    m_hasChanged = false;
    
    if (std::holds_alternative<int>(m_value) && std::holds_alternative<int>(m_previousValue)) {
        m_hasChanged = std::get<int>(m_value) != std::get<int>(m_previousValue);
    } else if (std::holds_alternative<float>(m_value) && std::holds_alternative<float>(m_previousValue)) {
        m_hasChanged = std::get<float>(m_value) != std::get<float>(m_previousValue);
    } else if (std::holds_alternative<std::vector<uint8_t>>(m_value) && 
              std::holds_alternative<std::vector<uint8_t>>(m_previousValue)) {
        m_hasChanged = std::get<std::vector<uint8_t>>(m_value) != std::get<std::vector<uint8_t>>(m_previousValue);
    } else {
        // Different types, consider it changed
        m_hasChanged = true;
    }
}

nlohmann::json MemoryMappingEntry::toJson() const
{
    nlohmann::json j;
    j["name"] = m_name;
    j["address"] = m_address;
    j["size"] = m_size;
    j["type"] = static_cast<int>(m_type);
    j["endianness"] = static_cast<int>(m_endianness);
    j["isArray"] = m_isArray;
    
    // Only export current value, not previous
    if (std::holds_alternative<int>(m_value)) {
        j["value"] = std::get<int>(m_value);
    } else if (std::holds_alternative<float>(m_value)) {
        j["value"] = std::get<float>(m_value);
    } else if (std::holds_alternative<std::vector<uint8_t>>(m_value)) {
        const auto& bytes = std::get<std::vector<uint8_t>>(m_value);
        nlohmann::json array = nlohmann::json::array();
        for (const auto& byte : bytes) {
            array.push_back(static_cast<int>(byte));
        }
        j["value"] = array;
    }
    
    return j;
}

// MappingGroup implementation
MappingGroup::MappingGroup(const std::string& name)
    : m_name(name)
    , m_entries()
{
}

std::string MappingGroup::getName() const
{
    return m_name;
}

void MappingGroup::addEntry(const MemoryMappingEntry& entry)
{
    m_entries.push_back(entry);
}

const std::vector<MemoryMappingEntry>& MappingGroup::getEntries() const
{
    return m_entries;
}

std::vector<MemoryMappingEntry>& MappingGroup::getEntries()
{
    return m_entries;
}

nlohmann::json MappingGroup::toJson() const
{
    nlohmann::json j;
    j["name"] = m_name;
    
    nlohmann::json entries = nlohmann::json::array();
    for (const auto& entry : m_entries) {
        entries.push_back(entry.toJson());
    }
    j["entries"] = entries;
    
    return j;
}

// AIMemoryMapping implementation
AIMemoryMapping::AIMemoryMapping()
    : m_groups()
    , m_externalMemoryRead(nullptr)
    , m_enableLogging(false)
{
}

AIMemoryMapping::~AIMemoryMapping()
{
}

bool AIMemoryMapping::loadMappingsFromFile(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open()) {
        if (m_enableLogging) {
            std::cerr << "Failed to open mapping file: " << filePath << std::endl;
        }
        return false;
    }
    
    try {
        nlohmann::json j;
        file >> j;
        
        m_groups.clear();
        
        for (const auto& group : j["groups"]) {
            std::string groupName = group["name"];
            MappingGroup mappingGroup(groupName);
            
            for (const auto& entry : group["entries"]) {
                std::string name = entry["name"];
                unsigned int address = entry["address"];
                unsigned int size = entry["size"];
                MemoryType type = static_cast<MemoryType>(entry["type"].get<int>());
                Endianness endianness = static_cast<Endianness>(entry["endianness"].get<int>());
                bool isArray = entry["isArray"];
                
                MemoryMappingEntry mappingEntry(name, address, size, type, endianness, isArray);
                mappingGroup.addEntry(mappingEntry);
            }
            
            m_groups.push_back(mappingGroup);
        }
        
        if (m_enableLogging) {
            std::cout << "Successfully loaded " << m_groups.size() << " mapping groups from " << filePath << std::endl;
        }
        
        return true;
    } catch (const std::exception& e) {
        if (m_enableLogging) {
            std::cerr << "Error parsing mapping file: " << e.what() << std::endl;
        }
        return false;
    }
}

bool AIMemoryMapping::saveMappingsToFile(const std::string& filePath) const
{
    try {
        nlohmann::json j;
        nlohmann::json groups = nlohmann::json::array();
        
        for (const auto& group : m_groups) {
            groups.push_back(group.toJson());
        }
        
        j["groups"] = groups;
        
        std::ofstream file(filePath);
        if (!file.is_open()) {
            if (m_enableLogging) {
                std::cerr << "Failed to open file for writing: " << filePath << std::endl;
            }
            return false;
        }
        
        file << j.dump(4);  // Pretty print with 4 spaces indentation
        
        if (m_enableLogging) {
            std::cout << "Successfully saved memory mappings to " << filePath << std::endl;
        }
        
        return true;
    } catch (const std::exception& e) {
        if (m_enableLogging) {
            std::cerr << "Error saving mapping file: " << e.what() << std::endl;
        }
        return false;
    }
}

void AIMemoryMapping::refreshValues()
{
    if (!m_externalMemoryRead) {
        if (m_enableLogging) {
            std::cerr << "No memory read function set" << std::endl;
        }
        return;
    }
    
    for (auto& group : m_groups) {
        for (auto& entry : group.getEntries()) {
            unsigned int address = entry.getAddress();
            unsigned int size = entry.getSize();
            MemoryType type = entry.getType();
            Endianness endianness = entry.getEndianness();
            
            std::vector<uint8_t> buffer(size);
            
            // Read memory using the external function
            bool success = m_externalMemoryRead(address, buffer.data(), size);
            
            if (!success) {
                if (m_enableLogging) {
                    std::cerr << "Failed to read memory at address " << address << " size " << size << std::endl;
                }
                continue;
            }
            
            // Process the read data based on type
            if (type == MemoryType::INT8) {
                if (size == 1) {
                    int8_t value = static_cast<int8_t>(buffer[0]);
                    entry.setValue(static_cast<int>(value));
                }
            } else if (type == MemoryType::INT16) {
                if (size == 2) {
                    int16_t value;
                    if (endianness == Endianness::LITTLE) {
                        value = static_cast<int16_t>(buffer[0] | (buffer[1] << 8));
                    } else {
                        value = static_cast<int16_t>((buffer[0] << 8) | buffer[1]);
                    }
                    entry.setValue(static_cast<int>(value));
                }
            } else if (type == MemoryType::INT32) {
                if (size == 4) {
                    int32_t value;
                    if (endianness == Endianness::LITTLE) {
                        value = buffer[0] | (buffer[1] << 8) | (buffer[2] << 16) | (buffer[3] << 24);
                    } else {
                        value = (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];
                    }
                    entry.setValue(static_cast<int>(value));
                }
            } else if (type == MemoryType::FLOAT32) {
                if (size == 4) {
                    float value;
                    if (endianness == Endianness::LITTLE) {
                        uint32_t intValue = buffer[0] | (buffer[1] << 8) | (buffer[2] << 16) | (buffer[3] << 24);
                        std::memcpy(&value, &intValue, sizeof(float));
                    } else {
                        uint32_t intValue = (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];
                        std::memcpy(&value, &intValue, sizeof(float));
                    }
                    entry.setValue(value);
                }
            } else if (type == MemoryType::BYTE_ARRAY) {
                entry.setValue(buffer);
            }
        }
    }
}

std::vector<MemoryMappingEntry> AIMemoryMapping::getChangedEntries() const
{
    std::vector<MemoryMappingEntry> changedEntries;
    
    for (const auto& group : m_groups) {
        for (const auto& entry : group.getEntries()) {
            if (entry.hasChanged()) {
                changedEntries.push_back(entry);
            }
        }
    }
    
    return changedEntries;
}

void AIMemoryMapping::setMemoryReadCallback(ExternalMemoryReadFunc callback)
{
    m_externalMemoryRead = callback;
}

void AIMemoryMapping::addGroup(const MappingGroup& group)
{
    m_groups.push_back(group);
}

const std::vector<MappingGroup>& AIMemoryMapping::getGroups() const
{
    return m_groups;
}

std::vector<MappingGroup>& AIMemoryMapping::getGroups()
{
    return m_groups;
}

void AIMemoryMapping::enableLogging(bool enable)
{
    m_enableLogging = enable;
}

MemoryMappingEntry* AIMemoryMapping::findEntryByName(const std::string& name)
{
    for (auto& group : m_groups) {
        for (auto& entry : group.getEntries()) {
            if (entry.getName() == name) {
                return &entry;
            }
        }
    }
    
    return nullptr;
}

nlohmann::json AIMemoryMapping::exportValuesToJson() const
{
    nlohmann::json j = nlohmann::json::object();
    
    for (const auto& group : m_groups) {
        nlohmann::json groupValues = nlohmann::json::object();
        
        for (const auto& entry : group.getEntries()) {
            if (std::holds_alternative<int>(entry.getValue())) {
                groupValues[entry.getName()] = std::get<int>(entry.getValue());
            } else if (std::holds_alternative<float>(entry.getValue())) {
                groupValues[entry.getName()] = std::get<float>(entry.getValue());
            } else if (std::holds_alternative<std::vector<uint8_t>>(entry.getValue())) {
                const auto& bytes = std::get<std::vector<uint8_t>>(entry.getValue());
                nlohmann::json array = nlohmann::json::array();
                for (const auto& byte : bytes) {
                    array.push_back(static_cast<int>(byte));
                }
                groupValues[entry.getName()] = array;
            }
        }
        
        j[group.getName()] = groupValues;
    }
    
    return j;
}

bool AIMemoryMapping::importValuesFromJson(const nlohmann::json& j)
{
    try {
        for (auto& group : m_groups) {
            const std::string& groupName = group.getName();
            
            if (j.contains(groupName) && j[groupName].is_object()) {
                const auto& groupJson = j[groupName];
                
                for (auto& entry : group.getEntries()) {
                    const std::string& entryName = entry.getName();
                    
                    if (groupJson.contains(entryName)) {
                        const auto& value = groupJson[entryName];
                        
                        if (std::holds_alternative<int>(entry.getValue()) && value.is_number_integer()) {
                            entry.setValue(value.get<int>());
                        } else if (std::holds_alternative<float>(entry.getValue()) && value.is_number()) {
                            entry.setValue(value.get<float>());
                        } else if (std::holds_alternative<std::vector<uint8_t>>(entry.getValue()) && value.is_array()) {
                            std::vector<uint8_t> bytes;
                            for (const auto& item : value) {
                                if (item.is_number_integer()) {
                                    bytes.push_back(static_cast<uint8_t>(item.get<int>()));
                                }
                            }
                            entry.setValue(bytes);
                        }
                    }
                }
            }
        }
        
        return true;
    } catch (const std::exception& e) {
        if (m_enableLogging) {
            std::cerr << "Error importing values from JSON: " << e.what() << std::endl;
        }
        return false;
    }
}

// Memory read functions based on architecture
AIMemoryMapping::MemoryReadFunc AIMemoryMapping::GetMemoryReadFunction(const std::string& architecture) {
    if (architecture == "CPS1" || architecture == "CPS2" || architecture == "CPS3") {
        // CPS ROM/RAM access
        return [](uint32_t address) -> uint8_t {
            if (address >= 0x01000000 && address < 0x02000000) {
                // Main RAM
                return CpsRam[address & 0xFFFFFF];
            } else if (address >= 0x02000000 && address < 0x03000000) {
                // CPU RAM
                return CpuRam[address & 0xFFFFFF];
            } else if (address >= 0x03000000 && address < 0x04000000) {
                // Z80 RAM
                return CpsZRam[address & 0xFFFFFF];
            } else if (address >= 0x04000000 && address < 0x05000000) {
                // Foreground RAM
                return CpsFrg[address & 0xFFFFFF];
            }
            // Default to 0 if out of bounds
            return 0;
        };
    } else if (architecture == "NEOGEO" || architecture == "M68K") {
        // M68K RAM access
        return [](uint32_t address) -> uint8_t {
            // Map addresses to correct regions
            if (address < 0x200000) {
                // M68K RAM
                return MainRam[address & 0x1FFFFF];
            }
            // Default to 0 if out of bounds
            return 0;
        };
    } else if (architecture == "Z80") {
        // Z80 RAM access
        return [](uint32_t address) -> uint8_t {
            if (address < 0x10000) {
                // Z80 RAM
                return Z80Ram[address & 0xFFFF];
            }
            // Default to 0 if out of bounds
            return 0;
        };
    }
    
    // Default (unknown architecture)
    return nullptr;
}

// Helper function to convert memory address string to integer
uint32_t AddressFromString(const std::string& addressStr) {
    try {
        // Handle hex format (0xXXXXXXXX)
        if (addressStr.size() > 2 && addressStr[0] == '0' && (addressStr[1] == 'x' || addressStr[1] == 'X')) {
            return static_cast<uint32_t>(std::stoul(addressStr.substr(2), nullptr, 16));
        }
        // Handle decimal format
        return static_cast<uint32_t>(std::stoul(addressStr));
    } catch (const std::exception& e) {
        std::cerr << "Error parsing address string: " << addressStr << " - " << e.what() << std::endl;
        return 0;
    }
}

// Helper function to convert mask string to integer
uint32_t MaskFromString(const std::string& maskStr) {
    try {
        // Handle hex format (0xXXXXXXXX)
        if (maskStr.size() > 2 && maskStr[0] == '0' && (maskStr[1] == 'x' || maskStr[1] == 'X')) {
            return static_cast<uint32_t>(std::stoul(maskStr.substr(2), nullptr, 16));
        }
        // Handle decimal format
        return static_cast<uint32_t>(std::stoul(maskStr));
    } catch (const std::exception& e) {
        std::cerr << "Error parsing mask string: " << maskStr << " - " << e.what() << std::endl;
        return 0;
    }
}

} // namespace AI 