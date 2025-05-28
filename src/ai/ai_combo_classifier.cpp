#include "ai_combo_classifier.h"
#include "ai_input_frame.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <nlohmann/json.hpp>
#include <sstream>

using json = nlohmann::json;

namespace AI {

AIComboClassifier::AIComboClassifier() 
    : m_isInitialized(false),
      m_minimumComboLength(2),
      m_comboTimeoutFrames(30),
      m_currentCombo(),
      m_combos(),
      m_activeCombo(false),
      m_lastHitFrame(0),
      m_maxComboLength(0),
      m_comboStartHealth(0) {
}

AIComboClassifier::~AIComboClassifier() {
    // Clean up resources if needed
}

bool AIComboClassifier::initialize(const std::string& comboDefinitionsPath) {
    try {
        std::ifstream file(comboDefinitionsPath);
        if (!file.is_open()) {
            std::cerr << "Failed to open combo definitions file: " << comboDefinitionsPath << std::endl;
            return false;
        }

        json jsonData;
        file >> jsonData;
        file.close();

        // Parse combo definitions
        for (const auto& comboJson : jsonData["combos"]) {
            ComboDefinition combo;
            combo.name = comboJson["name"];
            combo.description = comboJson.value("description", "");
            combo.difficulty = comboJson.value("difficulty", 1);
            combo.damage = comboJson.value("damage", 0);
            
            // Parse move sequence
            for (const auto& moveJson : comboJson["sequence"]) {
                ComboMove move;
                move.name = moveJson["name"];
                move.frameWindow = moveJson.value("frame_window", 10);
                move.requiredInputs = moveJson.value("inputs", std::vector<std::string>());
                combo.sequence.push_back(move);
            }
            
            m_comboDefinitions.push_back(combo);
        }

        // Parse configuration values if present
        if (jsonData.contains("config")) {
            const auto& config = jsonData["config"];
            m_minimumComboLength = config.value("minimum_combo_length", 2);
            m_comboTimeoutFrames = config.value("combo_timeout_frames", 30);
        }

        m_isInitialized = true;
        reset();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error initializing combo classifier: " << e.what() << std::endl;
        return false;
    }
}

bool AIComboClassifier::isInitialized() const {
    return m_isInitialized;
}

void AIComboClassifier::processFrame(const AIInputFrame& frame) {
    if (!m_isInitialized) return;

    // Track health history
    HealthSnapshot snapshot;
    snapshot.frameNumber = frame.getFrameNumber();
    snapshot.player1Health = frame.getP1Health();
    snapshot.player2Health = frame.getP2Health();
    
    m_healthHistory.push_back(snapshot);
    
    // Keep a reasonable window of health history
    if (m_healthHistory.size() > 120) {
        m_healthHistory.pop_front();
    }
    
    // Need at least 2 frames to detect health changes
    if (m_healthHistory.size() < 2) {
        return;
    }
    
    auto& current = m_healthHistory.back();
    auto& previous = m_healthHistory[m_healthHistory.size() - 2];
    
    // Check for health reduction (only track P2 taking damage for now)
    int healthChange = previous.player2Health - current.player2Health;
    
    if (healthChange > 0) {
        // Damage detected
        if (!m_activeCombo) {
            // Start new combo
            m_activeCombo = true;
            m_currentCombo.clear();
            m_comboStartHealth = previous.player2Health;
        }
        
        // Add hit to current combo
        ComboHit hit;
        hit.frameNumber = frame.getFrameNumber();
        hit.damage = healthChange;
        m_currentCombo.push_back(hit);
        
        m_lastHitFrame = frame.getFrameNumber();
    }
    else if (m_activeCombo) {
        // Check for combo timeout
        if (frame.getFrameNumber() - m_lastHitFrame > m_comboTimeoutFrames) {
            endCombo(frame.getFrameNumber());
        }
        
        // Check if player moved out of hitstun (simplified)
        if (frame.getP2State() == 0) { // 0 = neutral state (not in hitstun)
            endCombo(frame.getFrameNumber());
        }
    }
}

void AIComboClassifier::endCombo(int frameNumber) {
    if (m_activeCombo && m_currentCombo.size() >= m_minimumComboLength) {
        // Record the combo
        RecordedCombo combo;
        combo.hits = m_currentCombo;
        combo.startFrame = m_currentCombo.front().frameNumber;
        combo.endFrame = frameNumber;
        
        // Calculate total damage
        combo.totalDamage = 0;
        for (const auto& hit : m_currentCombo) {
            combo.totalDamage += hit.damage;
        }
        
        // Try to identify the combo
        combo.matchedDefinition = identifyCombo(m_currentCombo);
        
        // Save combo
        m_combos.push_back(combo);
        
        // Update max combo length
        m_maxComboLength = std::max(m_maxComboLength, static_cast<int>(m_currentCombo.size()));
    }
    
    m_activeCombo = false;
    m_currentCombo.clear();
}

std::string AIComboClassifier::identifyCombo(const std::vector<ComboHit>& hits) {
    // TODO: Implement combo pattern matching against definitions
    return "Unknown Combo";
}

bool AIComboClassifier::isComboActive() const {
    return m_activeCombo;
}

int AIComboClassifier::getCurrentComboLength() const {
    return static_cast<int>(m_currentCombo.size());
}

int AIComboClassifier::getMaxComboLength() const {
    return m_maxComboLength;
}

int AIComboClassifier::getTotalCombos() const {
    return static_cast<int>(m_combos.size());
}

const std::vector<AIComboClassifier::RecordedCombo>& AIComboClassifier::getCombos() const {
    return m_combos;
}

std::vector<AIComboClassifier::RecordedCombo> AIComboClassifier::getRecentCombos(int count) const {
    std::vector<RecordedCombo> recentCombos;
    
    size_t startIdx = (m_combos.size() > static_cast<size_t>(count)) 
        ? m_combos.size() - count 
        : 0;
        
    for (size_t i = startIdx; i < m_combos.size(); i++) {
        recentCombos.push_back(m_combos[i]);
    }
    
    return recentCombos;
}

std::string AIComboClassifier::exportCombosToJson() const {
    std::stringstream json;
    
    json << "{\n";
    json << "  \"combos\": [\n";
    
    for (size_t i = 0; i < m_combos.size(); i++) {
        const auto& combo = m_combos[i];
        
        json << "    {\n";
        json << "      \"startFrame\": " << combo.startFrame << ",\n";
        json << "      \"endFrame\": " << combo.endFrame << ",\n";
        json << "      \"totalDamage\": " << combo.totalDamage << ",\n";
        json << "      \"hitCount\": " << combo.hits.size() << ",\n";
        json << "      \"matchedDefinition\": \"" << combo.matchedDefinition << "\",\n";
        json << "      \"hits\": [\n";
        
        for (size_t j = 0; j < combo.hits.size(); j++) {
            const auto& hit = combo.hits[j];
            
            json << "        {\n";
            json << "          \"frameNumber\": " << hit.frameNumber << ",\n";
            json << "          \"damage\": " << hit.damage << "\n";
            json << "        }";
            
            if (j < combo.hits.size() - 1) {
                json << ",";
            }
            
            json << "\n";
        }
        
        json << "      ]\n";
        json << "    }";
        
        if (i < m_combos.size() - 1) {
            json << ",";
        }
        
        json << "\n";
    }
    
    json << "  ],\n";
    json << "  \"maxComboLength\": " << m_maxComboLength << ",\n";
    json << "  \"totalCombos\": " << m_combos.size() << "\n";
    json << "}\n";
    
    return json.str();
}

bool AIComboClassifier::saveCombosToFile(const std::string& filePath) const {
    try {
        std::ofstream outFile(filePath);
        if (!outFile.is_open()) {
            std::cerr << "Failed to open file for writing: " << filePath << std::endl;
            return false;
        }
        
        outFile << exportCombosToJson();
        outFile.close();
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error saving combos to file: " << e.what() << std::endl;
        return false;
    }
}

void AIComboClassifier::reset() {
    m_currentCombo.clear();
    m_combos.clear();
    m_healthHistory.clear();
    m_activeCombo = false;
    m_lastHitFrame = 0;
    m_maxComboLength = 0;
}

void AIComboClassifier::setMinimumComboLength(int length) {
    m_minimumComboLength = length;
}

int AIComboClassifier::getMinimumComboLength() const {
    return m_minimumComboLength;
}

void AIComboClassifier::setComboTimeoutFrames(int frames) {
    m_comboTimeoutFrames = frames;
}

int AIComboClassifier::getComboTimeoutFrames() const {
    return m_comboTimeoutFrames;
}

} // namespace AI 