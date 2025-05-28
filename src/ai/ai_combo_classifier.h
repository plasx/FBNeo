#pragma once

#include "ai_input_frame.h"
#include "ai_output_action.h"
#include <string>
#include <vector>
#include <deque>
#include <cstdint>
#include <map>
#include <memory>
#include <functional>

namespace AI {

// Forward declarations
class AIInputFrame;

/**
 * @brief Combo classifier for fighting games
 * 
 * The AIComboClassifier detects and tracks combos in fighting games,
 * using health changes to identify hits and tracking sequences to 
 * identify and classify combos.
 */
class AIComboClassifier {
public:
    using ComboDetectedCallback = std::function<void(const ComboPattern&, int, int)>;

    /**
     * @brief Structure representing a hit in a combo
     */
    struct ComboHit {
        int frameNumber;    ///< Frame when the hit occurred
        int damage;         ///< Damage dealt by the hit
    };

    /**
     * @brief Structure for recording health snapshots
     */
    struct HealthSnapshot {
        int frameNumber;    ///< Frame number 
        int player1Health;  ///< Player 1 health
        int player2Health;  ///< Player 2 health
    };

    /**
     * @brief Structure representing a complete combo
     */
    struct RecordedCombo {
        std::vector<ComboHit> hits;   ///< Sequence of hits in the combo
        int startFrame;               ///< First frame of the combo
        int endFrame;                 ///< Last frame of the combo
        int totalDamage;              ///< Total damage dealt by the combo
        std::string matchedDefinition; ///< Name of matched combo definition
    };

    /**
     * @brief Structure representing a combo move in a definition
     */
    struct ComboMove {
        std::string name;                     ///< Name of the move
        int frameWindow;                      ///< Window for input in frames
        std::vector<std::string> requiredInputs; ///< Required inputs for the move
    };

    /**
     * @brief Structure for a predefined combo
     */
    struct ComboDefinition {
        std::string name;                ///< Name of the combo
        std::string description;         ///< Description of the combo
        int difficulty;                  ///< Difficulty rating (1-10)
        int damage;                      ///< Expected damage
        std::vector<ComboMove> sequence; ///< Sequence of moves in the combo
    };

    /**
     * @brief Default constructor
     */
    AIComboClassifier();

    /**
     * @brief Destructor
     */
    ~AIComboClassifier();

    /**
     * @brief Initialize the combo classifier
     * 
     * @param comboDefinitionsPath Path to JSON file with combo definitions
     * @return true if initialized successfully, false otherwise
     */
    bool initialize(const std::string& comboDefinitionsPath);

    /**
     * @brief Check if the classifier is initialized
     * 
     * @return true if initialized, false otherwise
     */
    bool isInitialized() const;

    /**
     * @brief Process a frame to detect hits and combos
     * 
     * @param frame The current frame data
     */
    void processFrame(const AIInputFrame& frame);

    /**
     * @brief End the current combo
     * 
     * @param frameNumber The current frame number
     */
    void endCombo(int frameNumber);

    /**
     * @brief Identify a combo based on hit sequence
     * 
     * @param hits Vector of hits to identify
     * @return Name of identified combo
     */
    std::string identifyCombo(const std::vector<ComboHit>& hits);

    /**
     * @brief Check if a combo is currently active
     * 
     * @return true if a combo is active, false otherwise
     */
    bool isComboActive() const;

    /**
     * @brief Get the current combo length
     * 
     * @return Number of hits in the current combo
     */
    int getCurrentComboLength() const;

    /**
     * @brief Get the maximum combo length achieved
     * 
     * @return Maximum number of hits in any combo
     */
    int getMaxComboLength() const;

    /**
     * @brief Get the total number of combos detected
     * 
     * @return Number of combos
     */
    int getTotalCombos() const;

    /**
     * @brief Get all recorded combos
     * 
     * @return Vector of all recorded combos
     */
    const std::vector<RecordedCombo>& getCombos() const;

    /**
     * @brief Get the most recent combos
     * 
     * @param count Number of recent combos to get
     * @return Vector of recent combos
     */
    std::vector<RecordedCombo> getRecentCombos(int count) const;

    /**
     * @brief Export combos to JSON string
     * 
     * @return JSON string with combo data
     */
    std::string exportCombosToJson() const;

    /**
     * @brief Save combos to a file
     * 
     * @param filePath Path to save file
     * @return true if saved successfully, false otherwise
     */
    bool saveCombosToFile(const std::string& filePath) const;

    /**
     * @brief Reset combo state
     */
    void reset();

    /**
     * @brief Set minimum hits required for a combo
     * 
     * @param length Minimum number of hits
     */
    void setMinimumComboLength(int length);

    /**
     * @brief Get minimum hits required for a combo
     * 
     * @return Minimum number of hits
     */
    int getMinimumComboLength() const;

    /**
     * @brief Set frames before combo times out
     * 
     * @param frames Number of frames
     */
    void setComboTimeoutFrames(int frames);

    /**
     * @brief Get frames before combo times out
     * 
     * @return Number of frames
     */
    int getComboTimeoutFrames() const;

    /**
     * @brief Add a combo pattern to the classifier
     * @param pattern The combo pattern to add
     * @return True if successfully added, false otherwise
     */
    bool addComboPattern(const ComboPattern& pattern);

    /**
     * @brief Remove a combo pattern by name
     * @param name The name of the combo pattern to remove
     * @return True if successfully removed, false if not found
     */
    bool removeComboPattern(const std::string& name);

    /**
     * @brief Clear all combo patterns
     */
    void clearComboPatterns();

    /**
     * @brief Process an input action, detecting combos
     * @param action The input action to process
     * @param frameNumber The current frame number
     * @return True if a combo was detected, false otherwise
     */
    bool processAction(const AIOutputAction& action, int frameNumber);

    /**
     * @brief Get a list of all currently registered combo patterns
     * @return Vector of combo patterns
     */
    std::vector<ComboPattern> getComboPatterns() const;

    /**
     * @brief Load combo patterns from a JSON file
     * @param filename Path to the JSON file
     * @return Number of patterns loaded
     */
    int loadComboPatternsFromFile(const std::string& filename);

    /**
     * @brief Save combo patterns to a JSON file
     * @param filename Path to the JSON file
     * @return True if successful, false otherwise
     */
    bool saveComboPatternsToFile(const std::string& filename) const;

    /**
     * @brief Set callback for when a combo is detected
     * @param callback Function to call when a combo is detected
     */
    void setComboDetectedCallback(ComboDetectedCallback callback);

    /**
     * @brief Get the most recently detected combo
     * @return The most recently detected combo or empty pattern if none
     */
    ComboPattern getLastDetectedCombo() const;

    /**
     * @brief Check if a specific combo is currently in progress
     * @param comboName Name of the combo to check
     * @return True if the combo is in progress, false otherwise
     */
    bool isComboInProgress(const std::string& comboName) const;

    /**
     * @brief Get a list of all combos detected in the current session
     * @return Map of combo names to count of how many times detected
     */
    std::map<std::string, int> getDetectedComboCounts() const;

private:
    bool m_isInitialized;                      ///< Initialization state
    int m_minimumComboLength;                  ///< Minimum hits for a combo
    int m_comboTimeoutFrames;                  ///< Frames before combo times out
    std::vector<ComboHit> m_currentCombo;      ///< Current combo hits
    std::vector<RecordedCombo> m_combos;       ///< All recorded combos
    bool m_activeCombo;                        ///< Whether a combo is active
    int m_lastHitFrame;                        ///< Frame of last hit
    int m_maxComboLength;                      ///< Maximum combo length
    std::deque<HealthSnapshot> m_healthHistory; ///< Recent health values
    int m_comboStartHealth;                    ///< Health at combo start
    std::vector<ComboDefinition> m_comboDefinitions; ///< Predefined combos
    std::vector<ComboPattern> m_comboPatterns;
    std::vector<AIOutputAction> m_recentActions;
    ComboPattern m_lastDetectedCombo;
    ComboDetectedCallback m_comboDetectedCallback;
    std::map<std::string, int> m_detectedComboCounts;
    int m_maxHistorySize;
    int m_lastFrameNumber;
    
    // Helper methods
    bool checkForCombo(int frameNumber);
    bool matchesComboPattern(const ComboPattern& pattern, 
                             const std::vector<AIOutputAction>& actions,
                             int startIdx);
};

} // namespace AI 