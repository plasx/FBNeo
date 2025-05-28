#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <simd/simd.h>

#include "metal_debug_overlay.h"
#include "ai_memory_mapping.h"
#include "ai_input_frame.h"
#include "ai_output_action.h"
#include "combo_classifier.h"
#include "neural_ai_controller.h"

#include <string>
#include <vector>
#include <map>
#include <cmath>

// Initialize the singleton instance
MetalDebugOverlay* MetalDebugOverlay::s_instance = nullptr;

// Helper function to convert string to NSString
static NSString* NSStringFromStdString(const std::string& str) {
    return [NSString stringWithUTF8String:str.c_str()];
}

// Vertex structure for the debug overlay
struct DebugVertex {
    simd::float2 position;
    simd::float2 texCoord;
    simd::float4 color;
};

// Create a SIMD float4 from RGB values (0-255)
static simd::float4 RGBColor(float r, float g, float b, float a = 1.0f) {
    return simd::float4{r/255.0f, g/255.0f, b/255.0f, a};
}

#pragma mark - Initialization and Cleanup

MetalDebugOverlay* MetalDebugOverlay::Initialize(id<MTLDevice> device, id<MTLCommandQueue> commandQueue) {
    if (!s_instance) {
        s_instance = new MetalDebugOverlay(device, commandQueue);
    }
    
    return s_instance;
}

void MetalDebugOverlay::Shutdown() {
    if (s_instance) {
        delete s_instance;
        s_instance = nullptr;
    }
}

MetalDebugOverlay* MetalDebugOverlay::GetInstance() {
    return s_instance;
}

MetalDebugOverlay::MetalDebugOverlay(id<MTLDevice> device, id<MTLCommandQueue> commandQueue)
    : m_device(device),
      m_commandQueue(commandQueue),
      m_enabled(true),
      m_scale(1.0f),
      m_opacity(0.85f),
      m_nextPanelId(1),
      m_memoryMapping(nullptr),
      m_aiController(nullptr),
      m_comboClassifier(nullptr) {
    
    // Initialize training metrics
    m_trainingMetrics.progress = 0.0f;
    m_trainingMetrics.reward = 0.0f;
    m_trainingMetrics.loss = 0.0f;
    
    // Initialize resources
    InitializeResources();
    
    NSLog(@"MetalDebugOverlay: Initialized");
}

MetalDebugOverlay::~MetalDebugOverlay() {
    // Release Metal resources
    m_vertexBuffer = nil;
    m_uniformBuffer = nil;
    m_fontTexture = nil;
    m_pipelineState = nil;
    
    NSLog(@"MetalDebugOverlay: Destroyed");
}

void MetalDebugOverlay::InitializeResources() {
    // Create a default font texture (simplified for compilation)
    MTLTextureDescriptor *texDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                                                                       width:512
                                                                                      height:512
                                                                                   mipmapped:NO];
    m_fontTexture = [m_device newTextureWithDescriptor:texDesc];
    
    // Create a simple vertex buffer for rendering quads
    static const DebugVertex quadVertices[] = {
        // Position                 Texcoord               Color
        { {-1.0f, -1.0f}, {0.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f} },
        { { 1.0f, -1.0f}, {1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f} },
        { {-1.0f,  1.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f} },
        { { 1.0f,  1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f} }
    };
    
    m_vertexBuffer = [m_device newBufferWithBytes:quadVertices
                                           length:sizeof(quadVertices)
                                          options:MTLResourceStorageModeShared];
    
    // Create uniform buffer
    m_uniformBuffer = [m_device newBufferWithLength:sizeof(float) * 4
                                            options:MTLResourceStorageModeShared];
    
    // Create a pipeline state
    // NOTE: In a real implementation, we would load and compile shaders
    // This is simplified to just allow compilation to succeed
    
    NSLog(@"MetalDebugOverlay: Resources initialized");
}

// Stub implementation to allow compilation
void MetalDebugOverlay::Render(id<MTLRenderCommandEncoder> renderEncoder, float deltaTime) {
    if (!m_enabled || !renderEncoder) {
        return;
    }
    
    // Simplified for compilation
    NSLog(@"MetalDebugOverlay: Render called (stub)");
}

// Minimal implementation for all required methods to allow compilation
void MetalDebugOverlay::UpdateUniformBuffer(float deltaTime) {
    // Simplified for compilation
}

void MetalDebugOverlay::RenderPanel(const DebugPanelConfig& config, id<MTLRenderCommandEncoder> encoder, float deltaTime) {
    // Simplified for compilation
}

void MetalDebugOverlay::RenderMemoryPanel(const DebugPanelConfig& config, id<MTLRenderCommandEncoder> encoder) {
    // Simplified for compilation
}

void MetalDebugOverlay::RenderAIStatePanel(const DebugPanelConfig& config, id<MTLRenderCommandEncoder> encoder) {
    // Simplified for compilation
}

void MetalDebugOverlay::RenderTrainingPanel(const DebugPanelConfig& config, id<MTLRenderCommandEncoder> encoder) {
    // Simplified for compilation
}

void MetalDebugOverlay::RenderInputHistoryPanel(const DebugPanelConfig& config, id<MTLRenderCommandEncoder> encoder) {
    // Simplified for compilation
}

void MetalDebugOverlay::RenderComboPanel(const DebugPanelConfig& config, id<MTLRenderCommandEncoder> encoder) {
    // Simplified for compilation
}

void MetalDebugOverlay::RenderPerformancePanel(const DebugPanelConfig& config, id<MTLRenderCommandEncoder> encoder) {
    // Simplified for compilation
}

void MetalDebugOverlay::RenderMappingPanel(const DebugPanelConfig& config, id<MTLRenderCommandEncoder> encoder) {
    // Simplified for compilation
}

void MetalDebugOverlay::SetEnabled(bool enabled) {
    m_enabled = enabled;
}

bool MetalDebugOverlay::IsEnabled() const {
    return m_enabled;
}

void MetalDebugOverlay::ToggleVisibility() {
    m_enabled = !m_enabled;
}

void MetalDebugOverlay::SetScale(float scale) {
    m_scale = std::max(0.5f, std::min(2.0f, scale));
}

void MetalDebugOverlay::SetOpacity(float opacity) {
    m_opacity = std::max(0.1f, std::min(1.0f, opacity));
}

int MetalDebugOverlay::AddPanel(const DebugPanelConfig& config) {
    int panelId = m_nextPanelId++;
    m_panels[panelId] = config;
    return panelId;
}

void MetalDebugOverlay::RemovePanel(int panelId) {
    m_panels.erase(panelId);
}

void MetalDebugOverlay::UpdatePanelConfig(int panelId, const DebugPanelConfig& config) {
    if (m_panels.find(panelId) != m_panels.end()) {
        m_panels[panelId] = config;
    }
}

void MetalDebugOverlay::ShowPanel(int panelId, bool show) {
    if (m_panels.find(panelId) != m_panels.end()) {
        m_panels[panelId].visible = show;
    }
}

void MetalDebugOverlay::SetMemoryMapping(const AIMemoryMapping* mapping) {
    m_memoryMapping = mapping;
}

void MetalDebugOverlay::SetAIController(const NeuralAIController* controller) {
    m_aiController = controller;
}

void MetalDebugOverlay::SetComboClassifier(const AIComboClassifier* classifier) {
    m_comboClassifier = classifier;
}

void MetalDebugOverlay::AddInputFrame(const AIInputFrame& inputFrame) {
    m_inputHistory.push_back(inputFrame);
    
    // Keep history to a reasonable size
    const size_t MAX_HISTORY = 100;
    if (m_inputHistory.size() > MAX_HISTORY) {
        m_inputHistory.erase(m_inputHistory.begin(), m_inputHistory.begin() + (m_inputHistory.size() - MAX_HISTORY));
    }
}

void MetalDebugOverlay::SetTrainingProgress(float progress, float reward, float loss) {
    m_trainingMetrics.progress = progress;
    m_trainingMetrics.reward = reward;
    m_trainingMetrics.loss = loss;
    
    // Add to history
    m_trainingMetrics.rewardHistory.push_back(reward);
    m_trainingMetrics.lossHistory.push_back(loss);
    
    // Keep history to a reasonable size
    const size_t MAX_HISTORY = 100;
    if (m_trainingMetrics.rewardHistory.size() > MAX_HISTORY) {
        m_trainingMetrics.rewardHistory.erase(m_trainingMetrics.rewardHistory.begin());
        m_trainingMetrics.lossHistory.erase(m_trainingMetrics.lossHistory.begin());
    }
}

void MetalDebugOverlay::AddMetric(const std::string& name, float value) {
    m_trainingMetrics.customMetrics[name] = value;
}

void MetalDebugOverlay::RenderText(const std::string& text, float2 position, float4 color, float size) {
    // Simplified for compilation
}

void MetalDebugOverlay::RenderGraph(const GraphRenderContext& context) {
    // Simplified for compilation
}

void MetalDebugOverlay::RenderMemoryView(const MemoryVisContext& context, const uint8_t* memoryPtr, size_t memorySize) {
    // Simplified for compilation
}

void MetalDebugOverlay::ShowTrainingVisualization(bool show) {
    // Simplified for compilation
}

void MetalDebugOverlay::UpdateTrainingStats(const std::map<std::string, float>& stats) {
    m_trainingMetrics.customMetrics = stats;
}

void MetalDebugOverlay::RegisterCustomRenderCallback(std::function<void(id<MTLRenderCommandEncoder>, float)> callback) {
    m_customRenderCallbacks.push_back(callback);
} 