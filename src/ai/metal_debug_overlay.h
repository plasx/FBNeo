#pragma once

// Metal Debug Overlay for FBNeo AI
// Provides real-time visualization of AI state, memory mapping, and training progress

#include <string>
#include <vector>
#include <functional>
#include <map>
#include <simd/simd.h>

#include "ai_memory_mapping.h"
#include "ai_input_frame.h"
#include "ai_output_action.h"
#include "combo_classifier.h"
#include "neural_ai_controller.h"

// Forward declarations
class AIMemoryMapping;
class AIInputFrame;
class AIOutputAction;
class AIComboClassifier;
class NeuralAIController;

// Metal-specific typedefs and structures
typedef simd::float4 float4;
typedef simd::float2 float2;

// Forward declarations for Metal objects
#ifdef __OBJC__
@protocol MTLDevice;
@protocol MTLCommandQueue;
@protocol MTLRenderCommandEncoder;
@protocol MTLBuffer;
@protocol MTLTexture;
@protocol MTLRenderPipelineState;
#else
typedef void* id;
#define MTLDevice void
#define MTLCommandQueue void
#define MTLRenderCommandEncoder void
#define MTLBuffer void
#define MTLTexture void
#define MTLRenderPipelineState void
#endif

// Text rendering context
struct TextRenderContext {
    void* fontRef;
    float fontSize;
    float4 textColor;
    float2 position;
    float lineHeight;
};

// Graph data point
struct GraphDataPoint {
    float value;
    float4 color;
    std::string label;
};

// Graph rendering context
struct GraphRenderContext {
    float2 position;
    float2 size;
    float4 backgroundColor;
    float4 gridColor;
    float4 axisColor;
    std::vector<GraphDataPoint> dataPoints;
    std::string xAxisLabel;
    std::string yAxisLabel;
    std::string title;
    float minValue;
    float maxValue;
    bool autoScale;
};

// Memory visualization context
struct MemoryVisContext {
    float2 position;
    float2 size;
    float4 backgroundColor;
    int startAddress;
    int endAddress;
    bool showAscii;
    bool highlightChanges;
    std::vector<int> watchAddresses;
};

// Debug panel types
enum class DebugPanelType {
    MEMORY_VIEW,
    AI_STATE,
    TRAINING_PROGRESS,
    INPUT_HISTORY,
    COMBO_ANALYZER,
    PERFORMANCE_METRICS,
    MEMORY_MAPPING_VIEW,
    CUSTOM
};

// Debug panel configuration
struct DebugPanelConfig {
    DebugPanelType type;
    std::string title;
    float2 position;
    float2 size;
    bool visible;
    bool showHeader;
    bool resizable;
    bool scrollable;
    float4 backgroundColor;
    float4 headerColor;
    int refreshRateMs;
    std::function<void(void*)> customRenderFunc;
    void* userData;
};

/**
 * @brief Metal Debug Overlay for visualizing AI state and training progress
 * 
 * This class provides a comprehensive debug overlay for the Metal renderer
 * that shows real-time information about AI state, memory mapping, input history,
 * and training progress.
 */
class MetalDebugOverlay {
public:
    // Initialization and cleanup
    static MetalDebugOverlay* Initialize(id<MTLDevice> device, id<MTLCommandQueue> commandQueue);
    static void Shutdown();
    static MetalDebugOverlay* GetInstance();

    // Main rendering method - called from Metal render loop
    void Render(id<MTLRenderCommandEncoder> renderEncoder, float deltaTime);

    // Configuration
    void SetEnabled(bool enabled);
    bool IsEnabled() const;
    void ToggleVisibility();
    void SetScale(float scale);
    void SetOpacity(float opacity);

    // Debug panel management
    int AddPanel(const DebugPanelConfig& config);
    void RemovePanel(int panelId);
    void UpdatePanelConfig(int panelId, const DebugPanelConfig& config);
    void ShowPanel(int panelId, bool show);
    
    // Data sources
    void SetMemoryMapping(const AIMemoryMapping* mapping);
    void SetAIController(const NeuralAIController* controller);
    void SetComboClassifier(const AIComboClassifier* classifier);
    void AddInputFrame(const AIInputFrame& inputFrame);
    void SetTrainingProgress(float progress, float reward, float loss);
    void AddMetric(const std::string& name, float value);
    
    // Text rendering
    void RenderText(const std::string& text, float2 position, float4 color, float size = 14.0f);
    
    // Graph rendering
    void RenderGraph(const GraphRenderContext& context);
    
    // Memory visualization
    void RenderMemoryView(const MemoryVisContext& context, const uint8_t* memoryPtr, size_t memorySize);
    
    // Training visualization
    void ShowTrainingVisualization(bool show);
    void UpdateTrainingStats(const std::map<std::string, float>& stats);
    
    // Custom rendering
    void RegisterCustomRenderCallback(std::function<void(id<MTLRenderCommandEncoder>, float)> callback);
    
private:
    MetalDebugOverlay(id<MTLDevice> device, id<MTLCommandQueue> commandQueue);
    ~MetalDebugOverlay();
    
    // Metal resources
    id<MTLDevice> m_device;
    id<MTLCommandQueue> m_commandQueue;
    id<MTLRenderPipelineState> m_pipelineState;
    id<MTLBuffer> m_vertexBuffer;
    id<MTLBuffer> m_uniformBuffer;
    id<MTLTexture> m_fontTexture;
    
    // Overlay state
    bool m_enabled;
    float m_scale;
    float m_opacity;
    
    // Debug panels
    std::map<int, DebugPanelConfig> m_panels;
    int m_nextPanelId;
    
    // Data sources
    const AIMemoryMapping* m_memoryMapping;
    const NeuralAIController* m_aiController;
    const AIComboClassifier* m_comboClassifier;
    std::vector<AIInputFrame> m_inputHistory;
    
    // Training metrics
    struct TrainingMetrics {
        float progress;
        float reward;
        float loss;
        std::map<std::string, float> customMetrics;
        std::vector<float> rewardHistory;
        std::vector<float> lossHistory;
    } m_trainingMetrics;
    
    // Custom rendering callbacks
    std::vector<std::function<void(id<MTLRenderCommandEncoder>, float)>> m_customRenderCallbacks;
    
    // Singleton instance
    static MetalDebugOverlay* s_instance;
    
    // Helper methods
    void InitializeResources();
    void RenderPanel(const DebugPanelConfig& config, id<MTLRenderCommandEncoder> encoder, float deltaTime);
    void RenderMemoryPanel(const DebugPanelConfig& config, id<MTLRenderCommandEncoder> encoder);
    void RenderAIStatePanel(const DebugPanelConfig& config, id<MTLRenderCommandEncoder> encoder);
    void RenderTrainingPanel(const DebugPanelConfig& config, id<MTLRenderCommandEncoder> encoder);
    void RenderInputHistoryPanel(const DebugPanelConfig& config, id<MTLRenderCommandEncoder> encoder);
    void RenderComboPanel(const DebugPanelConfig& config, id<MTLRenderCommandEncoder> encoder);
    void RenderPerformancePanel(const DebugPanelConfig& config, id<MTLRenderCommandEncoder> encoder);
    void RenderMappingPanel(const DebugPanelConfig& config, id<MTLRenderCommandEncoder> encoder);
    
    // Metal shader helpers
    void UpdateUniformBuffer(float deltaTime);
}; 