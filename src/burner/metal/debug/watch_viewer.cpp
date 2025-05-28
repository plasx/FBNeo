#include "watch_viewer.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>

// Constructor
WatchViewer::WatchViewer()
    : m_metalContext(nullptr)
    , m_watchpointManager(nullptr)
    , m_architecture("")
    , m_highlightChanges(true)
    , m_showAllArchitectures(false)
    , m_newWatchpointAddress(0)
    , m_newWatchpointName("")
    , m_newWatchpointAccessType(WatchpointManager::AccessType::ReadWrite)
    , m_newWatchpointDataType(WatchpointManager::DataType::Byte)
    , m_newWatchpointCondition("")
    , m_isEditing(false)
    , m_editingWatchpointId(0)
{
}

// Destructor
WatchViewer::~WatchViewer()
{
}

// Initialize the watch viewer
bool WatchViewer::initialize(MetalContext* metalContext, std::shared_ptr<WatchpointManager> watchpointManager)
{
    m_metalContext = metalContext;
    m_watchpointManager = watchpointManager;
    m_highlightChanges = true;
    m_showAllArchitectures = false;
    m_isEditing = false;
    
    // Reset form state
    m_newWatchpointAddress = 0;
    m_newWatchpointName = "";
    m_newWatchpointAccessType = WatchpointManager::AccessType::ReadWrite;
    m_newWatchpointDataType = WatchpointManager::DataType::Byte;
    m_newWatchpointCondition = "";
    
    return (m_metalContext != nullptr && m_watchpointManager != nullptr);
}

// Update the watch viewer state
void WatchViewer::update(float deltaTime)
{
    // Nothing to update in this simple implementation
    // In a real implementation with UI, this would handle input events
}

// Render the watch viewer
void WatchViewer::render(float x, float y, float width, float height)
{
    if (!m_metalContext || !m_watchpointManager) {
        return;
    }
    
    // In a real implementation, this would use a UI library like ImGui
    // For now, we'll just output to the console as a placeholder
    
    std::cout << "WatchViewer rendering at (" << x << ", " << y << ") with size " 
              << width << "x" << height << std::endl;
    
    // Render the watchpoint list
    renderWatchpointList(x, y, width * 0.7f, height);
    
    // Render the watchpoint form
    renderWatchpointForm(x + width * 0.7f, y, width * 0.3f, height * 0.5f);
    
    // If we're editing a watchpoint, render its details
    if (m_isEditing) {
        WatchpointManager::Watchpoint watchpoint;
        if (m_watchpointManager->getWatchpoint(m_editingWatchpointId, watchpoint)) {
            renderWatchpointDetails(
                x + width * 0.7f, 
                y + height * 0.5f, 
                width * 0.3f, 
                height * 0.5f,
                watchpoint
            );
        }
    }
}

// Add a new watchpoint
uint32_t WatchViewer::addWatchpoint(
    uint32_t address, 
    const std::string& name,
    WatchpointManager::AccessType accessType,
    WatchpointManager::DataType dataType,
    const std::string& condition,
    const std::string& cpuArchitecture)
{
    if (!m_watchpointManager) {
        return 0;
    }
    
    // Calculate the size based on the data type
    uint32_t size = 1;
    switch (dataType) {
        case WatchpointManager::DataType::Byte:
            size = 1;
            break;
        case WatchpointManager::DataType::Word:
            size = 2;
            break;
        case WatchpointManager::DataType::DWord:
            size = 4;
            break;
        case WatchpointManager::DataType::QWord:
            size = 8;
            break;
    }
    
    // Use the active architecture if none specified
    std::string arch = cpuArchitecture.empty() ? m_architecture : cpuArchitecture;
    
    return m_watchpointManager->addWatchpoint(
        address, 
        size, 
        accessType, 
        dataType, 
        name, 
        condition, 
        true, 
        arch
    );
}

// Remove a watchpoint
bool WatchViewer::removeWatchpoint(uint32_t id)
{
    if (!m_watchpointManager) {
        return false;
    }
    
    return m_watchpointManager->removeWatchpoint(id);
}

// Enable or disable a watchpoint
bool WatchViewer::enableWatchpoint(uint32_t id, bool enabled)
{
    if (!m_watchpointManager) {
        return false;
    }
    
    return m_watchpointManager->enableWatchpoint(id, enabled);
}

// Set a condition for a watchpoint
bool WatchViewer::setWatchpointCondition(uint32_t id, const std::string& condition)
{
    if (!m_watchpointManager) {
        return false;
    }
    
    return m_watchpointManager->setWatchpointCondition(id, condition);
}

// Clear all watchpoints
void WatchViewer::clearAllWatchpoints()
{
    if (m_watchpointManager) {
        m_watchpointManager->clearAllWatchpoints();
    }
}

// Get all watchpoints
std::vector<WatchpointManager::Watchpoint> WatchViewer::getAllWatchpoints() const
{
    if (!m_watchpointManager) {
        return {};
    }
    
    return m_watchpointManager->getAllWatchpoints();
}

// Set the active CPU architecture
void WatchViewer::setArchitecture(const std::string& architecture)
{
    m_architecture = architecture;
}

// Get the active CPU architecture
std::string WatchViewer::getArchitecture() const
{
    return m_architecture;
}

// Set whether to highlight changed values
void WatchViewer::setHighlightChanges(bool highlight)
{
    m_highlightChanges = highlight;
}

// Get whether changes are highlighted
bool WatchViewer::getHighlightChanges() const
{
    return m_highlightChanges;
}

// Set whether to show all watchpoints or only the active architecture
void WatchViewer::setShowAllArchitectures(bool showAll)
{
    m_showAllArchitectures = showAll;
}

// Get whether all architectures are shown
bool WatchViewer::getShowAllArchitectures() const
{
    return m_showAllArchitectures;
}

// Save viewer settings to a JSON string
std::string WatchViewer::saveSettings() const
{
    // In a real implementation, this would serialize settings to JSON
    // For now, just return a placeholder
    return "{}";
}

// Load viewer settings from a JSON string
bool WatchViewer::loadSettings(const std::string& settings)
{
    // In a real implementation, this would deserialize settings from JSON
    // For now, just return success
    return true;
}

// Rendering helpers
void WatchViewer::renderWatchpointList(float x, float y, float width, float height)
{
    std::cout << "Watchpoints List:" << std::endl;
    
    auto watchpoints = getAllWatchpoints();
    
    // Filter by architecture if needed
    if (!m_showAllArchitectures && !m_architecture.empty()) {
        auto it = std::remove_if(watchpoints.begin(), watchpoints.end(),
            [this](const WatchpointManager::Watchpoint& wp) {
                return wp.cpuArchitecture != m_architecture;
            });
        watchpoints.erase(it, watchpoints.end());
    }
    
    if (watchpoints.empty()) {
        std::cout << "  No watchpoints defined." << std::endl;
        return;
    }
    
    // Sort by ID
    std::sort(watchpoints.begin(), watchpoints.end(),
        [](const WatchpointManager::Watchpoint& a, const WatchpointManager::Watchpoint& b) {
            return a.id < b.id;
        });
    
    // Print header
    std::cout << "  " 
              << std::setw(4) << "ID" << " | "
              << std::setw(16) << "Name" << " | "
              << std::setw(10) << "Address" << " | "
              << std::setw(8) << "Type" << " | "
              << std::setw(10) << "Access" << " | "
              << std::setw(10) << "Value" << " | "
              << std::setw(6) << "Status" << " | "
              << "Condition" << std::endl;
    
    std::cout << "  " << std::string(100, '-') << std::endl;
    
    // Print each watchpoint
    for (const auto& wp : watchpoints) {
        std::string valueStr = formatValue(wp.currentValue, wp.dataType);
        std::string changeIndicator = "";
        
        if (m_highlightChanges && wp.currentValue != wp.previousValue) {
            changeIndicator = " *";
        }
        
        std::cout << "  " 
                  << std::setw(4) << wp.id << " | "
                  << std::setw(16) << wp.name << " | "
                  << "0x" << std::hex << std::setw(8) << std::setfill('0') << wp.address << std::dec << std::setfill(' ') << " | "
                  << std::setw(8) << formatDataType(wp.dataType) << " | "
                  << std::setw(10) << formatAccessType(wp.accessType) << " | "
                  << std::setw(10) << valueStr << changeIndicator << " | "
                  << std::setw(6) << (wp.enabled ? "Enabled" : "Disabled") << " | "
                  << wp.condition << std::endl;
    }
}

void WatchViewer::renderWatchpointForm(float x, float y, float width, float height)
{
    std::cout << "Add New Watchpoint:" << std::endl;
    std::cout << "  Address: 0x" << std::hex << m_newWatchpointAddress << std::dec << std::endl;
    std::cout << "  Name: " << m_newWatchpointName << std::endl;
    std::cout << "  Access Type: " << formatAccessType(m_newWatchpointAccessType) << std::endl;
    std::cout << "  Data Type: " << formatDataType(m_newWatchpointDataType) << std::endl;
    std::cout << "  Condition: " << m_newWatchpointCondition << std::endl;
    std::cout << "  Architecture: " << (m_architecture.empty() ? "Any" : m_architecture) << std::endl;
}

void WatchViewer::renderWatchpointDetails(float x, float y, float width, float height, const WatchpointManager::Watchpoint& watchpoint)
{
    std::cout << "Watchpoint Details (ID: " << watchpoint.id << "):" << std::endl;
    std::cout << "  Name: " << watchpoint.name << std::endl;
    std::cout << "  Address: 0x" << std::hex << watchpoint.address << std::dec << std::endl;
    std::cout << "  Size: " << watchpoint.size << " bytes" << std::endl;
    std::cout << "  Access Type: " << formatAccessType(watchpoint.accessType) << std::endl;
    std::cout << "  Data Type: " << formatDataType(watchpoint.dataType) << std::endl;
    std::cout << "  Current Value: " << formatValue(watchpoint.currentValue, watchpoint.dataType) << std::endl;
    std::cout << "  Previous Value: " << formatValue(watchpoint.previousValue, watchpoint.dataType) << std::endl;
    std::cout << "  Condition: " << watchpoint.condition << std::endl;
    std::cout << "  Architecture: " << watchpoint.cpuArchitecture << std::endl;
    std::cout << "  Status: " << (watchpoint.enabled ? "Enabled" : "Disabled") << std::endl;
    std::cout << "  Triggered: " << (watchpoint.hasTriggered ? "Yes" : "No") << std::endl;
}

// Formatting helpers
std::string WatchViewer::formatAccessType(WatchpointManager::AccessType type) const
{
    switch (type) {
        case WatchpointManager::AccessType::Read:
            return "Read";
        case WatchpointManager::AccessType::Write:
            return "Write";
        case WatchpointManager::AccessType::ReadWrite:
            return "Read/Write";
        default:
            return "Unknown";
    }
}

std::string WatchViewer::formatDataType(WatchpointManager::DataType type) const
{
    switch (type) {
        case WatchpointManager::DataType::Byte:
            return "Byte";
        case WatchpointManager::DataType::Word:
            return "Word";
        case WatchpointManager::DataType::DWord:
            return "DWord";
        case WatchpointManager::DataType::QWord:
            return "QWord";
        default:
            return "Unknown";
    }
}

std::string WatchViewer::formatValue(uint64_t value, WatchpointManager::DataType type) const
{
    std::stringstream ss;
    ss << "0x";
    
    switch (type) {
        case WatchpointManager::DataType::Byte:
            ss << std::hex << std::setw(2) << std::setfill('0') << (value & 0xFF);
            break;
        case WatchpointManager::DataType::Word:
            ss << std::hex << std::setw(4) << std::setfill('0') << (value & 0xFFFF);
            break;
        case WatchpointManager::DataType::DWord:
            ss << std::hex << std::setw(8) << std::setfill('0') << (value & 0xFFFFFFFF);
            break;
        case WatchpointManager::DataType::QWord:
            ss << std::hex << std::setw(16) << std::setfill('0') << value;
            break;
    }
    
    return ss.str();
}

// UI event handlers
void WatchViewer::handleAddWatchpoint()
{
    // Add a new watchpoint using the form values
    addWatchpoint(
        m_newWatchpointAddress,
        m_newWatchpointName,
        m_newWatchpointAccessType,
        m_newWatchpointDataType,
        m_newWatchpointCondition,
        m_architecture
    );
    
    // Reset form
    m_newWatchpointAddress = 0;
    m_newWatchpointName = "";
    m_newWatchpointCondition = "";
}

void WatchViewer::handleEditWatchpoint(uint32_t id)
{
    m_isEditing = true;
    m_editingWatchpointId = id;
    
    // In a real implementation, this would populate the form with the watchpoint data
    WatchpointManager::Watchpoint watchpoint;
    if (m_watchpointManager->getWatchpoint(id, watchpoint)) {
        m_newWatchpointAddress = watchpoint.address;
        m_newWatchpointName = watchpoint.name;
        m_newWatchpointAccessType = watchpoint.accessType;
        m_newWatchpointDataType = watchpoint.dataType;
        m_newWatchpointCondition = watchpoint.condition;
    }
}

void WatchViewer::handleRemoveWatchpoint(uint32_t id)
{
    if (m_isEditing && m_editingWatchpointId == id) {
        m_isEditing = false;
    }
    
    removeWatchpoint(id);
}

void WatchViewer::handleCancelEdit()
{
    m_isEditing = false;
    
    // Reset form
    m_newWatchpointAddress = 0;
    m_newWatchpointName = "";
    m_newWatchpointCondition = "";
}

void WatchViewer::handleSaveEdit()
{
    if (!m_isEditing) {
        return;
    }
    
    // In a real implementation, this would update the watchpoint with the form data
    // For now, just apply the condition update
    setWatchpointCondition(m_editingWatchpointId, m_newWatchpointCondition);
    
    m_isEditing = false;
    
    // Reset form
    m_newWatchpointAddress = 0;
    m_newWatchpointName = "";
    m_newWatchpointCondition = "";
} 