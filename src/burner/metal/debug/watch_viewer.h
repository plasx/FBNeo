#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <cstdint>
#include "watchpoint_manager.h"

// Forward declarations
class MetalContext;

/**
 * @class WatchViewer
 * @brief Provides a UI for viewing and managing memory watchpoints
 * 
 * The WatchViewer allows users to create, edit, and delete watchpoints,
 * view watchpoint statuses, and observe memory value changes. It supports
 * different data types (byte, word, dword) and access types (read, write, read/write).
 */
class WatchViewer {
public:
    /**
     * @brief Constructor
     */
    WatchViewer();
    
    /**
     * @brief Destructor
     */
    ~WatchViewer();
    
    /**
     * @brief Initialize the watch viewer
     * 
     * @param metalContext Pointer to the Metal rendering context
     * @param watchpointManager Pointer to the watchpoint manager
     * @return true if initialization succeeded, false otherwise
     */
    bool initialize(MetalContext* metalContext, std::shared_ptr<WatchpointManager> watchpointManager);
    
    /**
     * @brief Update the watch viewer state
     * 
     * @param deltaTime Time elapsed since the last update
     */
    void update(float deltaTime);
    
    /**
     * @brief Render the watch viewer
     * 
     * @param x X-coordinate of the top-left corner
     * @param y Y-coordinate of the top-left corner
     * @param width Width of the viewer
     * @param height Height of the viewer
     */
    void render(float x, float y, float width, float height);
    
    /**
     * @brief Add a new watchpoint
     * 
     * @param address Memory address to watch
     * @param name Optional name for the watchpoint
     * @param accessType Type of access to watch (read, write, read/write)
     * @param dataType Data type to watch (byte, word, dword, qword)
     * @param condition Optional condition expression
     * @param cpuArchitecture CPU architecture identifier
     * @return uint32_t The ID of the created watchpoint, or 0 if creation failed
     */
    uint32_t addWatchpoint(uint32_t address, 
                           const std::string& name = "",
                           WatchpointManager::AccessType accessType = WatchpointManager::AccessType::ReadWrite,
                           WatchpointManager::DataType dataType = WatchpointManager::DataType::Byte,
                           const std::string& condition = "",
                           const std::string& cpuArchitecture = "");
    
    /**
     * @brief Remove a watchpoint
     * 
     * @param id The ID of the watchpoint to remove
     * @return true if removal succeeded, false otherwise
     */
    bool removeWatchpoint(uint32_t id);
    
    /**
     * @brief Enable or disable a watchpoint
     * 
     * @param id The ID of the watchpoint
     * @param enabled Whether the watchpoint should be enabled
     * @return true if the operation succeeded, false otherwise
     */
    bool enableWatchpoint(uint32_t id, bool enabled);
    
    /**
     * @brief Set a condition for a watchpoint
     * 
     * @param id The ID of the watchpoint
     * @param condition The condition expression
     * @return true if the operation succeeded, false otherwise
     */
    bool setWatchpointCondition(uint32_t id, const std::string& condition);
    
    /**
     * @brief Clear all watchpoints
     */
    void clearAllWatchpoints();
    
    /**
     * @brief Get all watchpoints
     * 
     * @return std::vector<WatchpointManager::Watchpoint> List of all watchpoints
     */
    std::vector<WatchpointManager::Watchpoint> getAllWatchpoints() const;
    
    /**
     * @brief Set the active CPU architecture
     * 
     * @param architecture The CPU architecture identifier
     */
    void setArchitecture(const std::string& architecture);
    
    /**
     * @brief Get the active CPU architecture
     * 
     * @return std::string The active CPU architecture
     */
    std::string getArchitecture() const;
    
    /**
     * @brief Set whether to highlight changed values
     * 
     * @param highlight Whether to highlight changes
     */
    void setHighlightChanges(bool highlight);
    
    /**
     * @brief Get whether changes are highlighted
     * 
     * @return true if changes are highlighted
     */
    bool getHighlightChanges() const;
    
    /**
     * @brief Set whether to show all watchpoints or only the active architecture
     * 
     * @param showAll Whether to show all watchpoints
     */
    void setShowAllArchitectures(bool showAll);
    
    /**
     * @brief Get whether all architectures are shown
     * 
     * @return true if all architectures are shown
     */
    bool getShowAllArchitectures() const;
    
    /**
     * @brief Save viewer settings to a JSON string
     * 
     * @return std::string JSON string containing settings
     */
    std::string saveSettings() const;
    
    /**
     * @brief Load viewer settings from a JSON string
     * 
     * @param settings JSON string containing settings
     * @return true if settings were loaded successfully
     */
    bool loadSettings(const std::string& settings);

private:
    // Pointer to the Metal context for rendering
    MetalContext* m_metalContext;
    
    // Pointer to the watchpoint manager
    std::shared_ptr<WatchpointManager> m_watchpointManager;
    
    // Active CPU architecture
    std::string m_architecture;
    
    // UI state
    bool m_highlightChanges;
    bool m_showAllArchitectures;
    
    // New watchpoint form state
    uint32_t m_newWatchpointAddress;
    std::string m_newWatchpointName;
    WatchpointManager::AccessType m_newWatchpointAccessType;
    WatchpointManager::DataType m_newWatchpointDataType;
    std::string m_newWatchpointCondition;
    
    // Edit state
    bool m_isEditing;
    uint32_t m_editingWatchpointId;
    
    // Rendering helpers
    void renderWatchpointList(float x, float y, float width, float height);
    void renderWatchpointForm(float x, float y, float width, float height);
    void renderWatchpointDetails(float x, float y, float width, float height, const WatchpointManager::Watchpoint& watchpoint);
    
    // Formatting helpers
    std::string formatAccessType(WatchpointManager::AccessType type) const;
    std::string formatDataType(WatchpointManager::DataType type) const;
    std::string formatValue(uint64_t value, WatchpointManager::DataType type) const;
    
    // UI event handlers
    void handleAddWatchpoint();
    void handleEditWatchpoint(uint32_t id);
    void handleRemoveWatchpoint(uint32_t id);
    void handleCancelEdit();
    void handleSaveEdit();
}; 