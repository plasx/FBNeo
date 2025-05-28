#include "memory_viewer.h"
#include "../metal_intf.h"
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>

class MemoryViewer::MemoryViewerPrivate {
public:
    MemoryViewerPrivate(MemoryViewer* owner)
        : m_owner(owner)
        , m_metalContext(nullptr)
        , m_fontHeight(15.0f)
        , m_charWidth(8.0f)
        , m_addressColumnWidth(80.0f)
        , m_hexColumnWidth(25.0f)
        , m_asciiColumnWidth(16.0f)
        , m_inEditMode(false)
        , m_editCursorPosition(0)
        , m_editNibblePosition(0)
    {
    }

    bool initialize(MetalContext* metalContext) {
        m_metalContext = metalContext;
        return true;
    }

    void update(float deltaTime) {
        handleInput();
    }

    void render(float x, float y, float width, float height) {
        if (!m_metalContext) return;
        
        renderLayout(x, y, width, height);
        
        // Calculate view area sizes
        float hexViewWidth = width;
        float hexViewHeight = height;
        float structViewWidth = width;
        float structViewHeight = height;
        
        switch (m_owner->getViewMode()) {
            case 0: // Hex only
                renderHexView(x, y, hexViewWidth, hexViewHeight);
                break;
            case 1: // Structured only
                renderStructuredView(x, y, structViewWidth, structViewHeight);
                break;
            case 2: // Both
                hexViewWidth = width * 0.6f;
                structViewWidth = width * 0.4f;
                renderHexView(x, y, hexViewWidth, hexViewHeight);
                renderStructuredView(x + hexViewWidth, y, structViewWidth, structViewHeight);
                break;
        }
    }

private:
    MemoryViewer* m_owner;
    MetalContext* m_metalContext;
    
    // UI layout properties
    float m_fontHeight;
    float m_charWidth;
    float m_addressColumnWidth;
    float m_hexColumnWidth;
    float m_asciiColumnWidth;
    
    // Editing state
    bool m_inEditMode;
    int m_editCursorPosition;
    int m_editNibblePosition; // 0 = high nibble, 1 = low nibble
    
    void renderLayout(float x, float y, float width, float height) {
        // Draw memory viewer background
        // m_metalContext->drawRect(x, y, width, height, 0.15f, 0.15f, 0.15f, 1.0f);
        
        // Draw memory viewer title bar
        // m_metalContext->drawRect(x, y, width, 25.0f, 0.2f, 0.2f, 0.2f, 1.0f);
        // m_metalContext->drawText("Memory Viewer", x + 10, y + 5, 1.0f, 1.0f, 1.0f, 1.0f);
        
        // Draw region selector dropdown
        // float dropdownWidth = 150.0f;
        // m_metalContext->drawRect(x + width - dropdownWidth - 10, y + 3, dropdownWidth, 20.0f, 0.25f, 0.25f, 0.25f, 1.0f);
        
        // Draw address input field
        // float addressFieldWidth = 100.0f;
        // m_metalContext->drawRect(x + width - dropdownWidth - addressFieldWidth - 20, y + 3, addressFieldWidth, 20.0f, 0.25f, 0.25f, 0.25f, 1.0f);
        
        // Draw view mode tabs
        // float tabWidth = 80.0f;
        // m_metalContext->drawRect(x + 10, y + 30, tabWidth, 20.0f, 0.25f, 0.25f, 0.25f, 1.0f);
        // m_metalContext->drawText("Hex", x + 15, y + 32, 1.0f, 1.0f, 1.0f, 1.0f);
        
        // m_metalContext->drawRect(x + 10 + tabWidth + 5, y + 30, tabWidth, 20.0f, 0.2f, 0.2f, 0.2f, 1.0f);
        // m_metalContext->drawText("Structured", x + 15 + tabWidth + 5, y + 32, 0.8f, 0.8f, 0.8f, 1.0f);
    }
    
    void renderHexView(float x, float y, float width, float height) {
        // Start position for content
        float contentY = y + 55.0f;
        
        // Calculate visible rows
        int visibleRows = static_cast<int>((height - 60.0f) / m_fontHeight);
        
        // Get current address
        uint32_t currentAddress = m_owner->m_currentAddress;
        
        // Calculate columns based on width and column count
        int columnsCount = m_owner->getColumnsCount();
        
        // Draw header
        // m_metalContext->drawRect(x, contentY, width, m_fontHeight, 0.2f, 0.2f, 0.2f, 1.0f);
        
        // Draw address column header
        // m_metalContext->drawText("Address", x + 5, contentY, 0.8f, 0.8f, 0.8f, 1.0f);
        
        // Draw hex columns headers
        float hexX = x + m_addressColumnWidth;
        for (int col = 0; col < columnsCount; col++) {
            std::stringstream ss;
            ss << std::uppercase << std::setw(2) << std::setfill('0') << std::hex << col;
            // m_metalContext->drawText(ss.str().c_str(), hexX + col * m_hexColumnWidth, contentY, 0.8f, 0.8f, 0.8f, 1.0f);
        }
        
        // Draw ASCII column header
        float asciiX = hexX + columnsCount * m_hexColumnWidth + 10.0f;
        // m_metalContext->drawText("ASCII", asciiX, contentY, 0.8f, 0.8f, 0.8f, 1.0f);
        
        // Move to first row position
        contentY += m_fontHeight + 5.0f;
        
        // Draw memory rows
        for (int row = 0; row < visibleRows; row++) {
            uint32_t rowAddress = currentAddress + row * columnsCount;
            
            // Draw row address
            std::stringstream ss;
            ss << std::uppercase << std::setw(8) << std::setfill('0') << std::hex << rowAddress;
            // m_metalContext->drawText(ss.str().c_str(), x + 5, contentY + row * m_fontHeight, 1.0f, 1.0f, 1.0f, 1.0f);
            
            // Draw hex values
            std::string asciiRow;
            for (int col = 0; col < columnsCount; col++) {
                uint32_t address = rowAddress + col;
                uint8_t value = m_owner->readByte(address);
                
                float hexValueX = hexX + col * m_hexColumnWidth;
                
                // Determine if this byte is selected
                bool isSelected = (address >= m_owner->m_selectionStart && address <= m_owner->m_selectionEnd);
                bool isEditCursor = (m_inEditMode && address == (m_owner->m_selectionStart + m_editCursorPosition));
                
                // Draw selection highlight
                if (isSelected) {
                    // m_metalContext->drawRect(hexValueX - 2, contentY + row * m_fontHeight - 2, m_hexColumnWidth, m_fontHeight + 2, 0.3f, 0.5f, 0.7f, 0.5f);
                }
                
                // Draw hex value
                ss.str("");
                ss << std::uppercase << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(value);
                
                if (isEditCursor) {
                    // Highlight the nibble being edited
                    std::string hexStr = ss.str();
                    
                    // Draw first nibble (with potential highlight)
                    // float r = (m_editNibblePosition == 0) ? 1.0f : 1.0f;
                    // float g = (m_editNibblePosition == 0) ? 1.0f : 1.0f;
                    // float b = (m_editNibblePosition == 0) ? 0.0f : 1.0f;
                    // m_metalContext->drawText(hexStr.substr(0, 1).c_str(), hexValueX, contentY + row * m_fontHeight, r, g, b, 1.0f);
                    
                    // Draw second nibble (with potential highlight)
                    // r = (m_editNibblePosition == 1) ? 1.0f : 1.0f;
                    // g = (m_editNibblePosition == 1) ? 1.0f : 1.0f;
                    // b = (m_editNibblePosition == 1) ? 0.0f : 1.0f;
                    // m_metalContext->drawText(hexStr.substr(1, 1).c_str(), hexValueX + m_charWidth, contentY + row * m_fontHeight, r, g, b, 1.0f);
                } else {
                    // m_metalContext->drawText(ss.str().c_str(), hexValueX, contentY + row * m_fontHeight, 1.0f, 1.0f, 1.0f, 1.0f);
                }
                
                // Add to ASCII row
                char c = static_cast<char>(value);
                if (c >= 32 && c <= 126) {
                    asciiRow += c;
                } else {
                    asciiRow += '.';
                }
            }
            
            // Draw ASCII representation
            // m_metalContext->drawText(asciiRow.c_str(), asciiX, contentY + row * m_fontHeight, 1.0f, 1.0f, 1.0f, 1.0f);
        }
        
        // Draw scrollbar
        // TODO: Implement scrollbar
    }
    
    void renderStructuredView(float x, float y, float width, float height) {
        if (m_owner->m_structViews.empty()) {
            // m_metalContext->drawText("No structured views defined.", x + 10, y + 60, 1.0f, 1.0f, 1.0f, 1.0f);
            return;
        }
        
        float contentY = y + 55.0f;
        
        // Draw structured view header
        // m_metalContext->drawRect(x, contentY, width, m_fontHeight, 0.2f, 0.2f, 0.2f, 1.0f);
        // m_metalContext->drawText("Structured View", x + 5, contentY, 0.8f, 0.8f, 0.8f, 1.0f);
        
        contentY += m_fontHeight + 5.0f;
        
        // Draw each structured view
        for (const auto& view : m_owner->m_structViews) {
            // Draw view name and type
            std::stringstream ss;
            ss << view.name << " (" << view.typeName << ") @ 0x" << std::uppercase << std::hex << view.address;
            // m_metalContext->drawText(ss.str().c_str(), x + 5, contentY, 0.9f, 0.9f, 0.9f, 1.0f);
            
            contentY += m_fontHeight + 5.0f;
            
            // Get the structure type
            auto typeIt = m_owner->m_structTypes.find(view.typeName);
            if (typeIt != m_owner->m_structTypes.end()) {
                const auto& structType = typeIt->second;
                
                // Draw each field
                for (const auto& field : structType.fields) {
                    const auto& fieldName = field.first;
                    const auto& fieldInfo = field.second;
                    
                    // Draw field name
                    // m_metalContext->drawText(fieldName.c_str(), x + 20, contentY, 0.8f, 0.8f, 0.8f, 1.0f);
                    
                    // Draw field value based on type
                    std::string valueStr = "Unknown type";
                    uint32_t fieldAddress = view.address + fieldInfo.offset;
                    
                    if (fieldInfo.type == "uint8") {
                        uint8_t value = m_owner->readByte(fieldAddress);
                        ss.str("");
                        ss << "0x" << std::uppercase << std::hex << static_cast<int>(value) << " (" << std::dec << static_cast<int>(value) << ")";
                        valueStr = ss.str();
                    } else if (fieldInfo.type == "uint16") {
                        uint16_t value = m_owner->readWord(fieldAddress);
                        ss.str("");
                        ss << "0x" << std::uppercase << std::hex << static_cast<int>(value) << " (" << std::dec << static_cast<int>(value) << ")";
                        valueStr = ss.str();
                    } else if (fieldInfo.type == "uint32") {
                        uint32_t value = m_owner->readDWord(fieldAddress);
                        ss.str("");
                        ss << "0x" << std::uppercase << std::hex << value << " (" << std::dec << value << ")";
                        valueStr = ss.str();
                    } else if (fieldInfo.type == "int8") {
                        int8_t value = static_cast<int8_t>(m_owner->readByte(fieldAddress));
                        ss.str("");
                        ss << std::dec << static_cast<int>(value);
                        valueStr = ss.str();
                    } else if (fieldInfo.type == "int16") {
                        int16_t value = static_cast<int16_t>(m_owner->readWord(fieldAddress));
                        ss.str("");
                        ss << std::dec << static_cast<int>(value);
                        valueStr = ss.str();
                    } else if (fieldInfo.type == "int32") {
                        int32_t value = static_cast<int32_t>(m_owner->readDWord(fieldAddress));
                        ss.str("");
                        ss << std::dec << value;
                        valueStr = ss.str();
                    } else if (fieldInfo.type == "char") {
                        char value = static_cast<char>(m_owner->readByte(fieldAddress));
                        ss.str("");
                        ss << "'" << (isprint(value) ? value : '.') << "' (0x" << std::uppercase << std::hex << static_cast<int>(value) << ")";
                        valueStr = ss.str();
                    } else if (fieldInfo.type == "float") {
                        uint32_t rawValue = m_owner->readDWord(fieldAddress);
                        float value = *reinterpret_cast<float*>(&rawValue);
                        ss.str("");
                        ss << std::fixed << std::setprecision(6) << value;
                        valueStr = ss.str();
                    }
                    
                    // m_metalContext->drawText(valueStr.c_str(), x + 150, contentY, 1.0f, 1.0f, 1.0f, 1.0f);
                    
                    contentY += m_fontHeight;
                }
                
                contentY += 10.0f; // Add some space between structures
            }
        }
    }
    
    void handleInput() {
        // TODO: Implement keyboard navigation and editing
        // This would include handling key presses for:
        // - Arrow keys to navigate
        // - Page Up/Down to scroll by pages
        // - Home/End to go to start/end of region
        // - Enter to toggle edit mode
        // - Tab to switch between hex and ASCII
        // - Hex keys (0-9, A-F) to edit values when in edit mode
        // - Escape to exit edit mode
    }
};

// MemoryViewer implementation
MemoryViewer::MemoryViewer()
    : m_private(std::make_unique<MemoryViewerPrivate>(this))
    , m_currentAddress(0)
    , m_selectionStart(0)
    , m_selectionEnd(0)
    , m_editable(false)
    , m_viewMode(0)  // Default to hex view only
    , m_columnsCount(16) // Default to 16 columns
{
}

MemoryViewer::~MemoryViewer() {
}

bool MemoryViewer::initialize(MetalContext* metalContext) {
    return m_private->initialize(metalContext);
}

void MemoryViewer::update(float deltaTime) {
    m_private->update(deltaTime);
}

void MemoryViewer::render(float x, float y, float width, float height) {
    m_private->render(x, y, width, height);
}

void MemoryViewer::setReadCallback(std::function<uint8_t(uint32_t)> callback) {
    m_readCallback = callback;
}

void MemoryViewer::setWriteCallback(std::function<void(uint32_t, uint8_t)> callback) {
    m_writeCallback = callback;
}

void MemoryViewer::defineRegion(const std::string& name, uint32_t startAddress, uint32_t size, const std::string& description) {
    MemoryRegion region;
    region.name = name;
    region.startAddress = startAddress;
    region.size = size;
    region.description = description;
    
    m_regions.push_back(region);
}

void MemoryViewer::defineStructuredType(const std::string& name, const std::unordered_map<std::string, uint32_t>& fields, const std::string& description) {
    StructType type;
    type.name = name;
    type.description = description;
    
    // Convert fields map to StructField entries
    for (const auto& pair : fields) {
        StructField field;
        field.name = pair.first;
        field.offset = pair.second;
        field.size = 1; // Default to 1 byte
        field.type = "uint8"; // Default to uint8
        
        type.fields[pair.first] = field;
    }
    
    m_structTypes[name] = type;
}

void MemoryViewer::defineStructuredView(uint32_t address, const std::string& typeName, const std::string& instanceName) {
    StructView view;
    view.name = instanceName;
    view.address = address;
    view.typeName = typeName;
    
    m_structViews.push_back(view);
}

void MemoryViewer::goToAddress(uint32_t address) {
    m_currentAddress = address;
    m_selectionStart = address;
    m_selectionEnd = address;
}

uint32_t MemoryViewer::searchValue(uint32_t value, int size, uint32_t startAddress) {
    // Start search from startAddress
    uint32_t address = startAddress;
    uint32_t maxAddress = 0xFFFFFF; // Adjust this based on architecture
    
    while (address <= maxAddress - size) {
        bool found = false;
        
        switch (size) {
            case 1:
                found = (readByte(address) == static_cast<uint8_t>(value));
                break;
            case 2:
                found = (readWord(address) == static_cast<uint16_t>(value));
                break;
            case 4:
                found = (readDWord(address) == value);
                break;
        }
        
        if (found) {
            return address;
        }
        
        address++;
    }
    
    return 0xFFFFFFFF; // Not found
}

void MemoryViewer::setEditable(bool editable) {
    m_editable = editable;
}

bool MemoryViewer::isEditable() const {
    return m_editable;
}

void MemoryViewer::setViewMode(int mode) {
    if (mode >= 0 && mode <= 2) {
        m_viewMode = mode;
    }
}

int MemoryViewer::getViewMode() const {
    return m_viewMode;
}

void MemoryViewer::setColumnsCount(int columns) {
    if (columns >= 1 && columns <= 32) {
        m_columnsCount = columns;
    }
}

int MemoryViewer::getColumnsCount() const {
    return m_columnsCount;
}

std::string MemoryViewer::saveSettings() const {
    // TODO: Implement serialization to JSON
    return "{}";
}

bool MemoryViewer::loadSettings(const std::string& settings) {
    // TODO: Implement deserialization from JSON
    return true;
}

uint8_t MemoryViewer::readByte(uint32_t address) {
    if (m_readCallback) {
        return m_readCallback(address);
    }
    return 0;
}

void MemoryViewer::writeByte(uint32_t address, uint8_t value) {
    if (m_writeCallback && m_editable) {
        m_writeCallback(address, value);
    }
}

uint16_t MemoryViewer::readWord(uint32_t address) {
    // Little-endian by default, can be customized if needed
    uint16_t lo = readByte(address);
    uint16_t hi = readByte(address + 1);
    return (hi << 8) | lo;
}

void MemoryViewer::writeWord(uint32_t address, uint16_t value) {
    // Little-endian by default
    writeByte(address, value & 0xFF);
    writeByte(address + 1, (value >> 8) & 0xFF);
}

uint32_t MemoryViewer::readDWord(uint32_t address) {
    // Little-endian by default
    uint32_t b0 = readByte(address);
    uint32_t b1 = readByte(address + 1);
    uint32_t b2 = readByte(address + 2);
    uint32_t b3 = readByte(address + 3);
    return (b3 << 24) | (b2 << 16) | (b1 << 8) | b0;
}

void MemoryViewer::writeDWord(uint32_t address, uint32_t value) {
    // Little-endian by default
    writeByte(address, value & 0xFF);
    writeByte(address + 1, (value >> 8) & 0xFF);
    writeByte(address + 2, (value >> 16) & 0xFF);
    writeByte(address + 3, (value >> 24) & 0xFF);
} 