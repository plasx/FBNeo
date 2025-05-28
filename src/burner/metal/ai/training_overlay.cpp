#include "training_overlay.h"
#include "overlay_renderer.h"
#include "hitbox_visualizer.h"
#include "input_display.h"
#include "frame_data_display.h"
#include "game_state_display.h"
#include "../metal_intf.h"
#include <iostream>
#include <fstream>
#include <algorithm>

// Initialize static instance
TrainingOverlay* TrainingOverlay::s_instance = nullptr;

// Singleton access method
TrainingOverlay* TrainingOverlay::getInstance() {
    if (!s_instance) {
        s_instance = new TrainingOverlay();
    }
    return s_instance;
}

// Constructor
TrainingOverlay::TrainingOverlay()
    : m_renderer(nullptr)
    , m_hitboxVisualizer(nullptr)
    , m_inputDisplay(nullptr)
    , m_frameDataDisplay(nullptr)
    , m_gameStateDisplay(nullptr)
    , m_memoryMapping(nullptr)
    , m_metalContext(nullptr)
    , m_hitboxesEnabled(true)
    , m_frameDataEnabled(true)
    , m_inputDisplayEnabled(true)
    , m_gameStateEnabled(true)
    , m_opacity(0.8f)
    , m_initialized(false)
{
}

// Destructor
TrainingOverlay::~TrainingOverlay()
{
    // Components will be cleaned up by unique_ptr
}

// Initialize the training overlay
bool TrainingOverlay::initialize(AIMemoryMapping* memoryMapping, MetalContext* metalContext)
{
    if (!memoryMapping || !metalContext) {
        std::cerr << "TrainingOverlay: Invalid memory mapping or Metal context" << std::endl;
        return false;
    }
    
    m_memoryMapping = memoryMapping;
    m_metalContext = metalContext;
    
    // Create and initialize renderer
    m_renderer = std::make_unique<OverlayRenderer>();
    if (!m_renderer->initialize(metalContext)) {
        std::cerr << "TrainingOverlay: Failed to initialize renderer" << std::endl;
        return false;
    }
    
    // Create and initialize hitbox visualizer
    m_hitboxVisualizer = std::make_unique<HitboxVisualizer>();
    if (!m_hitboxVisualizer->initialize(m_renderer.get(), m_memoryMapping)) {
        std::cerr << "TrainingOverlay: Failed to initialize hitbox visualizer" << std::endl;
        // Continue anyway, just without hitboxes
    }
    
    // Create and initialize input display
    m_inputDisplay = std::make_unique<InputDisplay>();
    if (!m_inputDisplay->initialize(m_renderer.get(), m_memoryMapping)) {
        std::cerr << "TrainingOverlay: Failed to initialize input display" << std::endl;
        // Continue anyway, just without input display
    }
    
    // Create and initialize frame data display
    m_frameDataDisplay = std::make_unique<FrameDataDisplay>();
    if (!m_frameDataDisplay->initialize(m_renderer.get(), m_memoryMapping)) {
        std::cerr << "TrainingOverlay: Failed to initialize frame data display" << std::endl;
        // Continue anyway, just without frame data display
    }
    
    // Create and initialize game state display
    m_gameStateDisplay = std::make_unique<GameStateDisplay>(m_memoryMapping);
    if (!m_gameStateDisplay->initialize(m_metalContext)) {
        std::cerr << "TrainingOverlay: Failed to initialize game state display" << std::endl;
        // Continue anyway, just without game state display
    }
    
    // Try to load saved settings
    loadSettings();
    
    // Update component opacity
    setOpacity(m_opacity);
    
    m_initialized = true;
    return true;
}

// Update all active overlay components
void TrainingOverlay::update(float deltaTime)
{
    if (!m_initialized) {
        return;
    }
    
    // Only update components that are enabled
    if (m_hitboxesEnabled && m_hitboxVisualizer) {
        m_hitboxVisualizer->update();
    }
    
    if (m_inputDisplayEnabled && m_inputDisplay) {
        m_inputDisplay->update();
    }
    
    if (m_frameDataEnabled && m_frameDataDisplay) {
        m_frameDataDisplay->update(deltaTime);
    }
    
    if (m_gameStateEnabled && m_gameStateDisplay) {
        m_gameStateDisplay->update(deltaTime);
    }
}

// Render all active overlay components
void TrainingOverlay::render(int width, int height)
{
    if (!m_initialized || !m_renderer) {
        return;
    }
    
    // Begin frame
    m_renderer->beginFrame(width, height);
    
    // Render enabled components
    if (m_hitboxesEnabled && m_hitboxVisualizer) {
        m_hitboxVisualizer->render();
    }
    
    if (m_inputDisplayEnabled && m_inputDisplay) {
        m_inputDisplay->render();
    }
    
    if (m_frameDataEnabled && m_frameDataDisplay) {
        m_frameDataDisplay->render(width, height, m_opacity);
    }
    
    if (m_gameStateEnabled && m_gameStateDisplay) {
        m_gameStateDisplay->render(width, height, m_opacity);
    }
    
    // End frame
    m_renderer->endFrame();
}

// Toggle hitbox visualization
void TrainingOverlay::setHitboxesEnabled(bool enabled)
{
    m_hitboxesEnabled = enabled;
}

// Check if hitboxes are enabled
bool TrainingOverlay::isHitboxesEnabled() const
{
    return m_hitboxesEnabled;
}

// Toggle frame data display
void TrainingOverlay::setFrameDataEnabled(bool enabled)
{
    m_frameDataEnabled = enabled;
    
    if (m_frameDataDisplay) {
        m_frameDataDisplay->setEnabled(enabled);
    }
}

// Check if frame data is enabled
bool TrainingOverlay::isFrameDataEnabled() const
{
    return m_frameDataEnabled;
}

// Toggle input display
void TrainingOverlay::setInputDisplayEnabled(bool enabled)
{
    m_inputDisplayEnabled = enabled;
}

// Check if input display is enabled
bool TrainingOverlay::isInputDisplayEnabled() const
{
    return m_inputDisplayEnabled;
}

// Toggle game state display
void TrainingOverlay::setGameStateEnabled(bool enabled)
{
    m_gameStateEnabled = enabled;
    
    if (m_gameStateDisplay) {
        m_gameStateDisplay->setEnabled(enabled);
    }
}

// Check if game state display is enabled
bool TrainingOverlay::isGameStateEnabled() const
{
    return m_gameStateEnabled;
}

// Set opacity for all overlay elements
void TrainingOverlay::setOpacity(float opacity)
{
    m_opacity = std::max(0.0f, std::min(1.0f, opacity));
    
    // Update opacity for all components
    if (m_hitboxVisualizer) {
        m_hitboxVisualizer->setOpacity(m_opacity);
    }
    
    if (m_inputDisplay) {
        m_inputDisplay->setOpacity(m_opacity);
    }
    
    // Frame data and game state handle opacity in their render methods
}

// Get current opacity
float TrainingOverlay::getOpacity() const
{
    return m_opacity;
}

// Save settings to a file
bool TrainingOverlay::saveSettings(const std::string& filename)
{
    // This is a simple implementation
    // In a real application, we would use a proper JSON or XML library
    try {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "TrainingOverlay: Failed to open settings file for writing: " << filename << std::endl;
            return false;
        }
        
        file << "{\n";
        file << "  \"hitboxesEnabled\": " << (m_hitboxesEnabled ? "true" : "false") << ",\n";
        file << "  \"frameDataEnabled\": " << (m_frameDataEnabled ? "true" : "false") << ",\n";
        file << "  \"inputDisplayEnabled\": " << (m_inputDisplayEnabled ? "true" : "false") << ",\n";
        file << "  \"gameStateEnabled\": " << (m_gameStateEnabled ? "true" : "false") << ",\n";
        file << "  \"opacity\": " << m_opacity << "\n";
        file << "}\n";
        
        // Save component-specific settings
        if (m_gameStateDisplay) {
            m_gameStateDisplay->saveSettings();
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "TrainingOverlay: Error saving settings: " << e.what() << std::endl;
        return false;
    }
}

// Load settings from a file
bool TrainingOverlay::loadSettings(const std::string& filename)
{
    // This is a simple implementation
    // In a real application, we would use a proper JSON or XML library
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            // File doesn't exist yet, which is fine
            return false;
        }
        
        // Very basic JSON parsing
        std::string line;
        while (std::getline(file, line)) {
            size_t pos;
            
            // Check for hitboxes setting
            if ((pos = line.find("\"hitboxesEnabled\":")) != std::string::npos) {
                pos = line.find("true", pos);
                m_hitboxesEnabled = (pos != std::string::npos);
            }
            
            // Check for frame data setting
            else if ((pos = line.find("\"frameDataEnabled\":")) != std::string::npos) {
                pos = line.find("true", pos);
                m_frameDataEnabled = (pos != std::string::npos);
                
                if (m_frameDataDisplay) {
                    m_frameDataDisplay->setEnabled(m_frameDataEnabled);
                }
            }
            
            // Check for input display setting
            else if ((pos = line.find("\"inputDisplayEnabled\":")) != std::string::npos) {
                pos = line.find("true", pos);
                m_inputDisplayEnabled = (pos != std::string::npos);
            }
            
            // Check for game state setting
            else if ((pos = line.find("\"gameStateEnabled\":")) != std::string::npos) {
                pos = line.find("true", pos);
                m_gameStateEnabled = (pos != std::string::npos);
                
                if (m_gameStateDisplay) {
                    m_gameStateDisplay->setEnabled(m_gameStateEnabled);
                }
            }
            
            // Check for opacity setting
            else if ((pos = line.find("\"opacity\":")) != std::string::npos) {
                pos = line.find(":", pos);
                if (pos != std::string::npos) {
                    // Extract the number
                    pos += 1;
                    size_t end = line.find_first_of(",}\n", pos);
                    if (end != std::string::npos) {
                        std::string valueStr = line.substr(pos, end - pos);
                        try {
                            float value = std::stof(valueStr);
                            setOpacity(value);
                        } catch (...) {
                            // Ignore parsing errors
                        }
                    }
                }
            }
        }
        
        // Load component-specific settings
        if (m_gameStateDisplay) {
            m_gameStateDisplay->loadSettings();
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "TrainingOverlay: Error loading settings: " << e.what() << std::endl;
        return false;
    }
} 