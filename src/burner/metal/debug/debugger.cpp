#include "debugger.h"
#include "../metal_intf.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

// Private implementation class
class Debugger::DebuggerPrivate {
public:
    DebuggerPrivate(Debugger* parent) : m_parent(parent), m_metalContext(nullptr) {}
    ~DebuggerPrivate() = default;

    bool initialize(MetalContext* metalContext) {
        if (!metalContext) {
            std::cerr << "Debugger initialization failed: Invalid Metal context" << std::endl;
            return false;
        }

        m_metalContext = metalContext;
        return true;
    }

    void update(float deltaTime) {
        // Update active components
    }

    void render(int width, int height) {
        if (!m_metalContext || !m_parent->isVisible()) {
            return;
        }

        // Layout components
        layoutComponents(width, height);

        // Render debug windows
        renderMainWindow(width, height);
        
        // Render status bar
        renderStatusBar(width, height);
    }

    void layoutComponents(int width, int height) {
        // Update component layouts based on current window size
        // This will define positioning and sizing of all debug windows
        m_debugWindowWidth = width * 0.8f;
        m_debugWindowHeight = height * 0.8f;
        m_debugWindowX = (width - m_debugWindowWidth) / 2;
        m_debugWindowY = (height - m_debugWindowHeight) / 2;
        
        // Status bar at the bottom
        m_statusBarHeight = 30;
        m_statusBarY = m_debugWindowY + m_debugWindowHeight - m_statusBarHeight;
        
        // Component layouts inside the main window
        float componentHeight = (m_debugWindowHeight - m_statusBarHeight) / 2;
        
        // Memory viewer takes left half of top row
        m_memoryViewerWidth = m_debugWindowWidth / 2;
        m_memoryViewerHeight = componentHeight;
        m_memoryViewerX = m_debugWindowX;
        m_memoryViewerY = m_debugWindowY;
        
        // Register viewer takes right half of top row
        m_registerViewerWidth = m_debugWindowWidth / 2;
        m_registerViewerHeight = componentHeight;
        m_registerViewerX = m_debugWindowX + m_memoryViewerWidth;
        m_registerViewerY = m_debugWindowY;
        
        // Disassembly takes full width of bottom row
        m_disassemblyWidth = m_debugWindowWidth;
        m_disassemblyHeight = componentHeight;
        m_disassemblyX = m_debugWindowX;
        m_disassemblyY = m_debugWindowY + componentHeight;
    }

    void renderMainWindow(int width, int height) {
        // Draw main debug window background
        /*
        m_metalContext->drawRect(
            m_debugWindowX, m_debugWindowY,
            m_debugWindowWidth, m_debugWindowHeight,
            0.15f, 0.15f, 0.15f, 0.95f
        );
        
        // Draw window border
        m_metalContext->drawRectOutline(
            m_debugWindowX, m_debugWindowY,
            m_debugWindowWidth, m_debugWindowHeight,
            0.5f, 0.5f, 0.5f, 1.0f, 2.0f
        );
        
        // Draw window title
        m_metalContext->drawText(
            "FBNeo Debugger", 
            m_debugWindowX + 10, m_debugWindowY + 10,
            1.0f, 1.0f, 1.0f, 1.0f
        );
        */
        // The above drawing code is commented out as we need to check the actual
        // metal_intf.h implementation to see what drawing functions are available
        
        // Render active components if they exist
        if (m_parent->m_memoryViewer) {
            m_parent->m_memoryViewer->render(
                m_memoryViewerX, m_memoryViewerY,
                m_memoryViewerWidth, m_memoryViewerHeight
            );
        }
        
        // Add rendering for other components when they're implemented
    }

    void renderStatusBar(int width, int height) {
        // Draw status bar
        // Show current architecture, execution state, etc.
        std::stringstream statusText;
        statusText << "Architecture: " << m_parent->getArchitecture() 
                  << " | State: " << (m_parent->isPaused() ? "Paused" : "Running");
        
        // Render status text when metal_intf drawing functions are confirmed
    }

    void handleInput() {
        // Process keyboard/mouse input for the debugger
        // Handle component focus changes, etc.
    }

private:
    Debugger* m_parent;
    MetalContext* m_metalContext;
    
    // Layout variables
    float m_debugWindowX;
    float m_debugWindowY;
    float m_debugWindowWidth;
    float m_debugWindowHeight;
    
    float m_statusBarHeight;
    float m_statusBarY;
    
    float m_memoryViewerX;
    float m_memoryViewerY;
    float m_memoryViewerWidth;
    float m_memoryViewerHeight;
    
    float m_registerViewerX;
    float m_registerViewerY;
    float m_registerViewerWidth;
    float m_registerViewerHeight;
    
    float m_disassemblyX;
    float m_disassemblyY;
    float m_disassemblyWidth;
    float m_disassemblyHeight;
};

// Main class implementation
Debugger::Debugger() : 
    m_private(new DebuggerPrivate(this)),
    m_memoryViewer(new MemoryViewer()),
    m_initialized(false),
    m_visible(false),
    m_paused(false),
    m_architecture("m68k") // Default to M68K as it's common in arcade systems
{
}

Debugger::~Debugger() {
    // Unique_ptr members will be automatically deleted
}

bool Debugger::initialize(MetalContext* metalContext) {
    if (m_initialized) {
        return true; // Already initialized
    }
    
    // Initialize private implementation
    if (!m_private->initialize(metalContext)) {
        return false;
    }
    
    // Initialize components
    if (m_memoryViewer && !m_memoryViewer->initialize(metalContext)) {
        std::cerr << "Failed to initialize memory viewer" << std::endl;
        return false;
    }
    
    // Other components will be initialized when implemented
    
    m_initialized = true;
    return true;
}

void Debugger::update(float deltaTime) {
    if (!m_initialized || !m_visible) {
        return;
    }
    
    // Update private implementation
    m_private->update(deltaTime);
    
    // Update components
    if (m_memoryViewer) {
        m_memoryViewer->update(deltaTime);
    }
    
    // Other components will be updated when implemented
}

void Debugger::render(int width, int height) {
    if (!m_initialized || !m_visible) {
        return;
    }
    
    m_private->render(width, height);
}

void Debugger::setVisible(bool visible) {
    m_visible = visible;
    
    // If becoming visible and we're not paused, pause the emulation
    if (m_visible && !m_paused) {
        pause();
    }
}

bool Debugger::isVisible() const {
    return m_visible;
}

void Debugger::setArchitecture(const std::string& architecture) {
    m_architecture = architecture;
    
    // Update components with the new architecture
    // This might affect disassembly format, register sets, etc.
}

std::string Debugger::getArchitecture() const {
    return m_architecture;
}

MemoryViewer* Debugger::getMemoryViewer() {
    return m_memoryViewer.get();
}

void Debugger::setMemoryReadCallback(std::function<uint8_t(uint32_t)> callback) {
    // Set callback in memory viewer
    if (m_memoryViewer) {
        m_memoryViewer->setReadCallback(callback);
    }
    
    // Set in other components that need memory access when implemented
}

void Debugger::setMemoryWriteCallback(std::function<void(uint32_t, uint8_t)> callback) {
    // Set callback in memory viewer
    if (m_memoryViewer) {
        m_memoryViewer->setWriteCallback(callback);
    }
    
    // Set in other components that need memory access when implemented
}

void Debugger::defineMemoryRegion(const std::string& name, uint32_t startAddress, 
                                uint32_t size, const std::string& description) {
    if (m_memoryViewer) {
        m_memoryViewer->defineRegion(name, startAddress, size, description);
    }
}

void Debugger::defineStructuredType(const std::string& name, 
                                  const std::unordered_map<std::string, uint32_t>& fields,
                                  const std::string& description) {
    if (m_memoryViewer) {
        m_memoryViewer->defineStructuredType(name, fields, description);
    }
}

void Debugger::defineStructuredView(uint32_t address, const std::string& typeName,
                                  const std::string& instanceName) {
    if (m_memoryViewer) {
        m_memoryViewer->defineStructuredView(address, typeName, instanceName);
    }
}

void Debugger::stepInstruction() {
    // Will be implemented when CPU integration is added
    // This should advance the CPU by one instruction
    std::cout << "Step instruction not yet implemented" << std::endl;
}

void Debugger::pause() {
    if (!m_paused) {
        m_paused = true;
        // Will add code to actually pause the emulation when integrating with CPU
        std::cout << "Emulation paused" << std::endl;
    }
}

void Debugger::resume() {
    if (m_paused) {
        m_paused = false;
        // Will add code to resume the emulation when integrating with CPU
        std::cout << "Emulation resumed" << std::endl;
        
        // If debugger is visible, hide it
        if (m_visible) {
            setVisible(false);
        }
    }
}

bool Debugger::isPaused() const {
    return m_paused;
}

bool Debugger::saveSettings(const std::string& filename) {
    // Save component settings
    bool success = true;
    
    try {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open file for saving debugger settings: " << filename << std::endl;
            return false;
        }
        
        // Save basic settings
        file << "{\n";
        file << "  \"architecture\": \"" << m_architecture << "\",\n";
        file << "  \"visible\": " << (m_visible ? "true" : "false") << ",\n";
        
        // Save component settings
        file << "  \"components\": {\n";
        
        // Memory viewer settings
        if (m_memoryViewer) {
            file << "    \"memoryViewer\": ";
            // This would call a method to get JSON settings from the memory viewer
            // For now, just add a placeholder
            file << "{}\n";
        }
        
        // Add other component settings when implemented
        
        file << "  }\n";
        file << "}\n";
        
        file.close();
    } catch (const std::exception& e) {
        std::cerr << "Exception saving debugger settings: " << e.what() << std::endl;
        success = false;
    }
    
    return success;
}

bool Debugger::loadSettings(const std::string& filename) {
    // Load component settings
    // This would parse the JSON file and apply settings to each component
    std::cout << "Loading settings from " << filename << " (not yet implemented)" << std::endl;
    return false;
}

void Debugger::layoutComponents(int width, int height) {
    m_private->layoutComponents(width, height);
}

void Debugger::handleInput() {
    m_private->handleInput();
}

void Debugger::renderMainWindow(int width, int height) {
    m_private->renderMainWindow(width, height);
}

void Debugger::renderStatusBar(int width, int height) {
    m_private->renderStatusBar(width, height);
} 