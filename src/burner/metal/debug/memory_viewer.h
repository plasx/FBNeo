#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <cstdint>

// Forward declarations
class MetalContext;

/**
 * @class MemoryViewer
 * @brief Provides a hexadecimal and structured view of emulated system memory
 * 
 * The MemoryViewer allows examining and editing memory contents in both
 * hexadecimal format and through structured type views. It supports defining
 * memory regions, creating structured type definitions, and providing
 * read/write access to memory.
 */
class MemoryViewer {
public:
    /**
     * @brief Construct a new Memory Viewer object
     */
    MemoryViewer();
    
    /**
     * @brief Destroy the Memory Viewer object
     */
    ~MemoryViewer();
    
    /**
     * @brief Initialize the memory viewer
     * 
     * @param metalContext Pointer to the Metal rendering context
     * @return true if initialization succeeded, false otherwise
     */
    bool initialize(MetalContext* metalContext);
    
    /**
     * @brief Update the memory viewer state
     * 
     * @param deltaTime Time elapsed since the last update
     */
    void update(float deltaTime);
    
    /**
     * @brief Render the memory viewer
     * 
     * @param x X-coordinate of the top-left corner
     * @param y Y-coordinate of the top-left corner
     * @param width Width of the viewer
     * @param height Height of the viewer
     */
    void render(float x, float y, float width, float height);
    
    /**
     * @brief Set the callback function used to read memory
     * 
     * @param callback Function that takes an address and returns the byte at that address
     */
    void setReadCallback(std::function<uint8_t(uint32_t)> callback);
    
    /**
     * @brief Set the callback function used to write memory
     * 
     * @param callback Function that takes an address and a byte value to write
     */
    void setWriteCallback(std::function<void(uint32_t, uint8_t)> callback);
    
    /**
     * @brief Define a named memory region
     * 
     * @param name Name of the region
     * @param startAddress Starting address of the region
     * @param size Size of the region in bytes
     * @param description Optional description of the region
     */
    void defineRegion(const std::string& name, uint32_t startAddress, 
                      uint32_t size, const std::string& description = "");
    
    /**
     * @brief Define a structured type for viewing memory
     * 
     * @param name Name of the structured type
     * @param fields Map of field names to their byte offsets within the structure
     * @param description Optional description of the structured type
     */
    void defineStructuredType(const std::string& name, 
                             const std::unordered_map<std::string, uint32_t>& fields,
                             const std::string& description = "");
    
    /**
     * @brief Define a view of memory using a structured type
     * 
     * @param address Base address for the structured view
     * @param typeName Name of the structured type to use
     * @param instanceName Name for this particular instance of the structured view
     */
    void defineStructuredView(uint32_t address, const std::string& typeName,
                             const std::string& instanceName);
    
    /**
     * @brief Go to a specific memory address
     * 
     * @param address The address to navigate to
     */
    void goToAddress(uint32_t address);
    
    /**
     * @brief Search for a specific value in memory
     * 
     * @param value The value to search for
     * @param size The size of the value (1, 2, or 4 bytes)
     * @param startAddress The address to start searching from
     * @return uint32_t The first address where the value was found, or 0xFFFFFFFF if not found
     */
    uint32_t searchValue(uint32_t value, int size, uint32_t startAddress = 0);
    
    /**
     * @brief Set whether the memory viewer is in edit mode
     * 
     * @param editable True if memory should be editable, false for read-only
     */
    void setEditable(bool editable);
    
    /**
     * @brief Check if the memory viewer is in edit mode
     * 
     * @return true if memory is editable, false if read-only
     */
    bool isEditable() const;
    
    /**
     * @brief Set whether to show hex view, structured view, or both
     * 
     * @param mode 0 = hex only, 1 = structured only, 2 = both
     */
    void setViewMode(int mode);
    
    /**
     * @brief Get the current view mode
     * 
     * @return int Current view mode (0 = hex only, 1 = structured only, 2 = both)
     */
    int getViewMode() const;
    
    /**
     * @brief Set the number of columns in the hex view
     * 
     * @param columns Number of byte columns to display
     */
    void setColumnsCount(int columns);
    
    /**
     * @brief Get the current number of columns in the hex view
     * 
     * @return int Number of byte columns
     */
    int getColumnsCount() const;
    
    /**
     * @brief Save memory viewer settings to a JSON object
     * 
     * @return std::string JSON string containing settings
     */
    std::string saveSettings() const;
    
    /**
     * @brief Load memory viewer settings from a JSON object
     * 
     * @param settings JSON string containing settings
     * @return true if settings were loaded successfully, false otherwise
     */
    bool loadSettings(const std::string& settings);

private:
    // Forward declaration of private implementation class
    class MemoryViewerPrivate;
    
    // Private implementation instance
    std::unique_ptr<MemoryViewerPrivate> m_private;
    
    // Memory region definition
    struct MemoryRegion {
        std::string name;
        uint32_t startAddress;
        uint32_t size;
        std::string description;
    };
    
    // Structure for field definition
    struct StructField {
        std::string name;
        uint32_t offset;
        int size;       // Size in bytes (1, 2, or 4)
        std::string type; // "uint8", "uint16", "uint32", "int8", "int16", "int32", "float", "char"
    };
    
    // Structure type definition
    struct StructType {
        std::string name;
        std::unordered_map<std::string, StructField> fields;
        std::string description;
    };
    
    // Structured view instance
    struct StructView {
        std::string name;
        uint32_t address;
        std::string typeName;
    };
    
    // Data members
    std::vector<MemoryRegion> m_regions;
    std::unordered_map<std::string, StructType> m_structTypes;
    std::vector<StructView> m_structViews;
    
    std::function<uint8_t(uint32_t)> m_readCallback;
    std::function<void(uint32_t, uint8_t)> m_writeCallback;
    
    uint32_t m_currentAddress;
    uint32_t m_selectionStart;
    uint32_t m_selectionEnd;
    
    bool m_editable;
    int m_viewMode;     // 0 = hex only, 1 = structured only, 2 = both
    int m_columnsCount; // Number of columns in hex view
    
    // Private helper methods
    void renderHexView(float x, float y, float width, float height);
    void renderStructuredView(float x, float y, float width, float height);
    void handleInput();
    uint8_t readByte(uint32_t address);
    void writeByte(uint32_t address, uint8_t value);
    uint16_t readWord(uint32_t address);
    void writeWord(uint32_t address, uint16_t value);
    uint32_t readDWord(uint32_t address);
    void writeDWord(uint32_t address, uint32_t value);
}; 