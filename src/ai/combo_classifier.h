#pragma once

#include <vector>
#include <string>
#include <map>
#include <functional>
#include <cstdint>
#include <unordered_map>
#include <memory>

#include "ai_memory_mapping.h"
#include "ai_input_frame.h"
#include "ai_output_action.h"

// Forward declarations
class AIMemoryMapping;
class AIInputFrame;
class AIOutputAction;

namespace AI {

/**
 * @brief Combo classifier and scoring system for FBNeo AI
 * 
 * This system analyzes gameplay to detect and score combos, which provides:
 * 1. Reward signals for reinforcement learning
 * 2. Diversity metrics for exploration strategies
 * 3. Evaluation criteria for model performance
 */
class AIComboClassifier {
public:
    struct ComboEvent {
        int frameStart;
        int frameEnd;
        int hitCount;
        int damage;
        float complexity;
        std::vector<AIOutputAction> actions;
        std::string description;
        bool isReversal;
        bool isCounter;
        bool isFinisher;
        bool isEfficientUse; // Optimal resource usage
        
        ComboEvent() : frameStart(0), frameEnd(0), hitCount(0), damage(0), 
                       complexity(0.0f), isReversal(false), isCounter(false),
                       isFinisher(false), isEfficientUse(false) {}
    };
    
    struct ComboMetrics {
        int totalCombos;
        int maxComboHits;
        int maxComboDamage;
        float averageComplexity;
        int reversalCount;
        int counterHitCount;
        float diversityScore;
        float efficiencyScore;
        
        ComboMetrics() : totalCombos(0), maxComboHits(0), maxComboDamage(0),
                         averageComplexity(0.0f), reversalCount(0), 
                         counterHitCount(0), diversityScore(0.0f),
                         efficiencyScore(0.0f) {}
    };

    AIComboClassifier();
    ~AIComboClassifier();
    
    // Initialize with game-specific mapping info
    void Initialize(const AIMemoryMapping* mapping);
    
    // Process a frame to detect and track combos
    void ProcessFrame(const AIMemoryMapping* mapping, int frameNumber, const AIInputFrame& inputFrame);
    
    // Get current combo state 
    bool IsInCombo() const;
    int GetCurrentComboHits() const;
    int GetCurrentComboDamage() const;
    
    // Get historical combo data
    const std::vector<ComboEvent>& GetComboHistory() const;
    ComboMetrics GetMetrics() const;
    float GetDiversityScore() const;
    
    // Reset combo state and optionally clear history
    void Reset(bool clearHistory = false);
    
    // Save/load combo state for deterministic replay
    bool SaveState(const std::string& filename) const;
    bool LoadState(const std::string& filename);
    
    // Export combo data for analysis
    bool ExportToJson(const std::string& filename) const;
    
    // Set callback for combo detection
    void SetComboCallback(std::function<void(const ComboEvent&)> callback);

private:
    // Combo detection state
    bool m_inCombo;
    int m_comboStartFrame;
    int m_lastHitFrame;
    int m_hitCounter;
    int m_totalDamage;
    int m_lastOpponentHealth;
    std::vector<AIOutputAction> m_comboActions;
    
    // Game-specific combo detection parameters
    int m_maxFramesBetweenHits;
    int m_minHitsForCombo;
    
    // Combo history
    std::vector<ComboEvent> m_comboHistory;
    
    // Callback for notifying when combos are detected
    std::function<void(const ComboEvent&)> m_comboCallback;
    
    // Internal methods
    bool DetectComboStart(const AIMemoryMapping* mapping);
    bool DetectComboEnd(const AIMemoryMapping* mapping);
    float CalculateComplexity(const std::vector<AIOutputAction>& actions) const;
    bool IsReversal(const AIMemoryMapping* mapping) const;
    bool IsCounterHit(const AIMemoryMapping* mapping) const;
    
    // Diversity scoring
    std::map<std::string, int> m_comboTypeCount;
    float CalculateDiversityScore() const;
};

/**
 * ComboClassifier - Detects and classifies combos in fighting games
 * 
 * This class analyzes sequences of input frames to detect when combos are being performed,
 * classifies the type of combo, and tracks statistics about combo usage and effectiveness.
 */
class ComboClassifier {
public:
    // Combo type classification
    enum class ComboType {
        NONE,
        BASIC,
        SPECIAL,
        SUPER,
        CUSTOM
    };
    
    // Combo detection result
    struct ComboResult {
        bool isCombo;
        ComboType type;
        int damage;
        int hitCount;
        std::string name;
        float executionScore; // 0.0 to 1.0, how well the combo was executed
        
        ComboResult() : isCombo(false), type(ComboType::NONE), damage(0), 
                       hitCount(0), name(""), executionScore(0.0f) {}
    };
    
    // Combo definition for the classifier to recognize
    struct ComboDefinition {
        std::string name;
        ComboType type;
        std::vector<uint32_t> inputSequence; // Expected button sequence
        int expectedDamage;
        int expectedHitCount;
        
        ComboDefinition() : name(""), type(ComboType::NONE), 
                           expectedDamage(0), expectedHitCount(0) {}
    };

public:
    // Constructor and destructor
    ComboClassifier();
    ~ComboClassifier();
    
    // Initialize the classifier with game-specific combo definitions
    bool initialize(const std::string& gameId);
    
    // Process a new frame and check for combos
    ComboResult processFrame(const AIInputFrame& frame);
    
    // Add a new combo definition
    void addComboDefinition(const ComboDefinition& combo);
    
    // Clear all combo definitions
    void clearComboDefinitions();
    
    // Load combo definitions from a JSON file
    bool loadComboDefinitions(const std::string& filePath);
    
    // Save current combo definitions to a JSON file
    bool saveComboDefinitions(const std::string& filePath);
    
    // Get statistics about combo usage
    std::unordered_map<std::string, int> getComboUsageStats() const;
    
    // Reset combo detection state (call between rounds/matches)
    void reset();

private:
    // Track combo state
    bool m_comboInProgress;
    int m_currentComboHits;
    int m_currentComboDamage;
    int m_lastHealthValue;
    int m_comboStartFrame;
    int m_lastHitFrame;
    
    // Track input history for pattern matching
    std::vector<uint32_t> m_inputHistory;
    std::vector<AIInputFrame> m_frameHistory;
    
    // Combo definitions
    std::vector<ComboDefinition> m_comboDefinitions;
    
    // Game-specific information
    std::string m_currentGameId;
    
    // Stats tracking
    std::unordered_map<std::string, int> m_comboUsage;
    
    // Helper methods
    bool detectComboStart(const AIInputFrame& frame);
    bool detectComboEnd(const AIInputFrame& frame);
    ComboResult classifyCombo();
    void updateInputHistory(const AIInputFrame& frame);
    bool matchComboPattern(const std::vector<uint32_t>& pattern);
};

} // namespace AI 