#include "combo_classifier.h"
#include <cmath>
#include <algorithm>
#include <fstream>
#include <json/json.h> // Using jsoncpp for JSON serialization

AIComboClassifier::AIComboClassifier() :
    m_inCombo(false),
    m_comboStartFrame(0),
    m_lastHitFrame(0),
    m_hitCounter(0),
    m_totalDamage(0),
    m_lastOpponentHealth(0),
    m_maxFramesBetweenHits(15), // Default, will be overridden by game-specific values
    m_minHitsForCombo(2)        // Default minimum hits to count as a combo
{
}

AIComboClassifier::~AIComboClassifier() {
}

void AIComboClassifier::Initialize(const AIMemoryMapping* mapping) {
    if (!mapping) return;
    
    // Reset state
    Reset(true);
    
    // Get game-specific combo detection parameters from mapping metadata
    std::string gameId = mapping->GetGameId();
    
    // Different fighting games have different combo timing windows
    if (gameId.find("sf2") != std::string::npos) {
        m_maxFramesBetweenHits = 10;
    } else if (gameId.find("sf3") != std::string::npos) {
        m_maxFramesBetweenHits = 12; 
    } else if (gameId.find("mvc") != std::string::npos) {
        m_maxFramesBetweenHits = 18;
    } else if (gameId.find("kof") != std::string::npos) {
        m_maxFramesBetweenHits = 14;
    } else if (gameId.find("samsho") != std::string::npos) {
        m_maxFramesBetweenHits = 8;
    } else {
        // Default fallback
        m_maxFramesBetweenHits = 15;
    }
    
    printf("AIComboClassifier: Initialized for game %s (max frames between hits: %d)\n", 
           gameId.c_str(), m_maxFramesBetweenHits);
}

void AIComboClassifier::ProcessFrame(const AIMemoryMapping* mapping, int frameNumber, const AIInputFrame& inputFrame) {
    if (!mapping) return;
    
    // Get current health values and hit state from memory mapping
    int playerHealth = mapping->GetValueAsInt("p1_health", 0);
    int opponentHealth = mapping->GetValueAsInt("p2_health", 0);
    bool hitConnected = mapping->GetValueAsBool("p1_hit_connected", false) || 
                        mapping->GetValueAsBool("p2_stun", false);
    bool playerAttacking = mapping->GetValueAsBool("p1_attacking", false);
    
    // Store current input action
    if (frameNumber > 0 && playerAttacking) {
        AIOutputAction currentAction(inputFrame.GetRawInputs());
        m_comboActions.push_back(currentAction);
    }
    
    // Check for combo start
    if (!m_inCombo && hitConnected && playerAttacking) {
        m_inCombo = true;
        m_comboStartFrame = frameNumber;
        m_lastHitFrame = frameNumber;
        m_hitCounter = 1;
        m_lastOpponentHealth = opponentHealth;
        m_totalDamage = m_lastOpponentHealth - opponentHealth;
        m_comboActions.clear();
        
        printf("AIComboClassifier: Combo started at frame %d\n", frameNumber);
    }
    // Check for ongoing combo
    else if (m_inCombo) {
        // If hit connected, update combo info
        if (hitConnected && playerAttacking && m_lastOpponentHealth > opponentHealth) {
            int frameDiff = frameNumber - m_lastHitFrame;
            
            // If within combo timing window, continue combo
            if (frameDiff <= m_maxFramesBetweenHits) {
                m_hitCounter++;
                m_totalDamage += (m_lastOpponentHealth - opponentHealth);
                m_lastHitFrame = frameNumber;
                m_lastOpponentHealth = opponentHealth;
                
                printf("AIComboClassifier: Combo hit %d at frame %d (damage: %d)\n", 
                       m_hitCounter, frameNumber, m_totalDamage);
            }
            // If outside timing window, end previous combo and start new one
            else {
                ComboEvent event = EndCurrentCombo(frameNumber - 1);
                if (m_comboCallback) {
                    m_comboCallback(event);
                }
                
                // Start new combo
                m_inCombo = true;
                m_comboStartFrame = frameNumber;
                m_lastHitFrame = frameNumber;
                m_hitCounter = 1;
                m_lastOpponentHealth = opponentHealth;
                m_totalDamage = m_lastOpponentHealth - opponentHealth;
                m_comboActions.clear();
                
                printf("AIComboClassifier: New combo started at frame %d\n", frameNumber);
            }
        }
        // Check if combo has ended
        else {
            int frameDiff = frameNumber - m_lastHitFrame;
            bool opponentBlocking = mapping->GetValueAsBool("p2_blocking", false);
            bool opponentInHitstun = mapping->GetValueAsBool("p2_hitstun", false);
            bool playerWhiffed = mapping->GetValueAsBool("p1_whiffed", false);
            
            // End combo if:
            // 1. Too much time passed since last hit
            // 2. Opponent is no longer in hitstun and not blocking
            // 3. Player whiffed an attack
            if (frameDiff > m_maxFramesBetweenHits || 
                (!opponentInHitstun && !opponentBlocking) || 
                playerWhiffed) {
                
                // Only record combo if it meets minimum hit requirement
                if (m_hitCounter >= m_minHitsForCombo) {
                    ComboEvent event = EndCurrentCombo(frameNumber);
                    m_comboHistory.push_back(event);
                    
                    // Notify callback
                    if (m_comboCallback) {
                        m_comboCallback(event);
                    }
                    
                    printf("AIComboClassifier: Combo ended at frame %d (%d hits, %d damage)\n", 
                           frameNumber, m_hitCounter, m_totalDamage);
                } else {
                    // Reset without recording if too few hits
                    printf("AIComboClassifier: Short sequence ended (not recorded)\n");
                }
                
                m_inCombo = false;
                m_comboActions.clear();
            }
        }
    }
}

bool AIComboClassifier::IsInCombo() const {
    return m_inCombo;
}

int AIComboClassifier::GetCurrentComboHits() const {
    return m_hitCounter;
}

int AIComboClassifier::GetCurrentComboDamage() const {
    return m_totalDamage;
}

const std::vector<AIComboClassifier::ComboEvent>& AIComboClassifier::GetComboHistory() const {
    return m_comboHistory;
}

AIComboClassifier::ComboMetrics AIComboClassifier::GetMetrics() const {
    ComboMetrics metrics;
    
    metrics.totalCombos = m_comboHistory.size();
    
    if (metrics.totalCombos == 0) {
        return metrics;
    }
    
    // Calculate metrics from combo history
    metrics.maxComboHits = 0;
    metrics.maxComboDamage = 0;
    float totalComplexity = 0.0f;
    
    for (const auto& combo : m_comboHistory) {
        metrics.maxComboHits = std::max(metrics.maxComboHits, combo.hitCount);
        metrics.maxComboDamage = std::max(metrics.maxComboDamage, combo.damage);
        totalComplexity += combo.complexity;
        
        if (combo.isReversal) metrics.reversalCount++;
        if (combo.isCounter) metrics.counterHitCount++;
    }
    
    metrics.averageComplexity = totalComplexity / static_cast<float>(metrics.totalCombos);
    metrics.diversityScore = CalculateDiversityScore();
    
    // Calculate efficiency score (optimal resource usage)
    metrics.efficiencyScore = 0.0f;
    int totalEfficientCombos = 0;
    for (const auto& combo : m_comboHistory) {
        if (combo.isEfficientUse) {
            totalEfficientCombos++;
        }
    }
    
    if (metrics.totalCombos > 0) {
        metrics.efficiencyScore = static_cast<float>(totalEfficientCombos) / static_cast<float>(metrics.totalCombos);
    }
    
    return metrics;
}

float AIComboClassifier::GetDiversityScore() const {
    return CalculateDiversityScore();
}

void AIComboClassifier::Reset(bool clearHistory) {
    m_inCombo = false;
    m_comboStartFrame = 0;
    m_lastHitFrame = 0;
    m_hitCounter = 0;
    m_totalDamage = 0;
    m_lastOpponentHealth = 0;
    m_comboActions.clear();
    
    if (clearHistory) {
        m_comboHistory.clear();
        m_comboTypeCount.clear();
    }
}

bool AIComboClassifier::SaveState(const std::string& filename) const {
    try {
        Json::Value root;
        
        // Save current combo state
        root["inCombo"] = m_inCombo;
        root["comboStartFrame"] = m_comboStartFrame;
        root["lastHitFrame"] = m_lastHitFrame;
        root["hitCounter"] = m_hitCounter;
        root["totalDamage"] = m_totalDamage;
        root["lastOpponentHealth"] = m_lastOpponentHealth;
        
        // Save combo history
        Json::Value history(Json::arrayValue);
        for (const auto& combo : m_comboHistory) {
            Json::Value comboObj;
            comboObj["frameStart"] = combo.frameStart;
            comboObj["frameEnd"] = combo.frameEnd;
            comboObj["hitCount"] = combo.hitCount;
            comboObj["damage"] = combo.damage;
            comboObj["complexity"] = combo.complexity;
            comboObj["description"] = combo.description;
            comboObj["isReversal"] = combo.isReversal;
            comboObj["isCounter"] = combo.isCounter;
            comboObj["isFinisher"] = combo.isFinisher;
            comboObj["isEfficientUse"] = combo.isEfficientUse;
            
            history.append(comboObj);
        }
        root["comboHistory"] = history;
        
        // Save to file
        std::ofstream file(filename);
        if (!file.is_open()) {
            return false;
        }
        
        file << root;
        file.close();
        
        return true;
    } catch (const std::exception& e) {
        printf("AIComboClassifier: Error saving state: %s\n", e.what());
        return false;
    }
}

bool AIComboClassifier::LoadState(const std::string& filename) {
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            return false;
        }
        
        Json::Value root;
        file >> root;
        file.close();
        
        // Load current combo state
        m_inCombo = root["inCombo"].asBool();
        m_comboStartFrame = root["comboStartFrame"].asInt();
        m_lastHitFrame = root["lastHitFrame"].asInt();
        m_hitCounter = root["hitCounter"].asInt();
        m_totalDamage = root["totalDamage"].asInt();
        m_lastOpponentHealth = root["lastOpponentHealth"].asInt();
        
        // Load combo history
        m_comboHistory.clear();
        const Json::Value& history = root["comboHistory"];
        for (const auto& comboObj : history) {
            ComboEvent combo;
            combo.frameStart = comboObj["frameStart"].asInt();
            combo.frameEnd = comboObj["frameEnd"].asInt();
            combo.hitCount = comboObj["hitCount"].asInt();
            combo.damage = comboObj["damage"].asInt();
            combo.complexity = comboObj["complexity"].asFloat();
            combo.description = comboObj["description"].asString();
            combo.isReversal = comboObj["isReversal"].asBool();
            combo.isCounter = comboObj["isCounter"].asBool();
            combo.isFinisher = comboObj["isFinisher"].asBool();
            combo.isEfficientUse = comboObj["isEfficientUse"].asBool();
            
            m_comboHistory.push_back(combo);
        }
        
        // Recalculate combo type counts for diversity scoring
        m_comboTypeCount.clear();
        for (const auto& combo : m_comboHistory) {
            m_comboTypeCount[combo.description]++;
        }
        
        return true;
    } catch (const std::exception& e) {
        printf("AIComboClassifier: Error loading state: %s\n", e.what());
        return false;
    }
}

bool AIComboClassifier::ExportToJson(const std::string& filename) const {
    return SaveState(filename);
}

void AIComboClassifier::SetComboCallback(std::function<void(const ComboEvent&)> callback) {
    m_comboCallback = callback;
}

bool AIComboClassifier::DetectComboStart(const AIMemoryMapping* mapping) {
    if (!mapping) return false;
    
    bool hitConnected = mapping->GetValueAsBool("p1_hit_connected", false);
    bool playerAttacking = mapping->GetValueAsBool("p1_attacking", false);
    
    return hitConnected && playerAttacking;
}

bool AIComboClassifier::DetectComboEnd(const AIMemoryMapping* mapping) {
    if (!mapping) return false;
    
    bool opponentBlocking = mapping->GetValueAsBool("p2_blocking", false);
    bool opponentInHitstun = mapping->GetValueAsBool("p2_hitstun", false);
    
    return !opponentInHitstun && !opponentBlocking;
}

float AIComboClassifier::CalculateComplexity(const std::vector<AIOutputAction>& actions) const {
    if (actions.empty()) return 0.0f;
    
    // Count unique inputs
    std::set<std::string> uniqueInputs;
    for (const auto& action : actions) {
        uniqueInputs.insert(action.GetInputString());
    }
    
    // Calculate directional changes
    int directionChanges = 0;
    std::string lastDirection = "";
    for (const auto& action : actions) {
        std::string currentDirection = action.GetDirectionString();
        if (!lastDirection.empty() && currentDirection != lastDirection) {
            directionChanges++;
        }
        lastDirection = currentDirection;
    }
    
    // Calculate timing precision (consistent timing gets lower penalty)
    float timingVariance = 0.0f;
    if (actions.size() > 2) {
        std::vector<int> frameGaps;
        for (size_t i = 1; i < actions.size(); i++) {
            frameGaps.push_back(actions[i].GetFrameNumber() - actions[i-1].GetFrameNumber());
        }
        
        // Calculate variance
        float mean = 0.0f;
        for (int gap : frameGaps) {
            mean += gap;
        }
        mean /= frameGaps.size();
        
        float variance = 0.0f;
        for (int gap : frameGaps) {
            variance += (gap - mean) * (gap - mean);
        }
        timingVariance = variance / frameGaps.size();
    }
    
    // Complexity formula:
    // - More unique inputs increases complexity
    // - More direction changes increases complexity
    // - More consistent timing (lower variance) decreases complexity
    float complexity = (uniqueInputs.size() * 1.5f) + 
                       (directionChanges * 1.2f) + 
                       (std::min(1.0f, timingVariance / 100.0f) * 0.7f);
    
    return complexity;
}

bool AIComboClassifier::IsReversal(const AIMemoryMapping* mapping) const {
    if (!mapping) return false;
    
    bool wasHit = mapping->GetValueAsBool("p1_was_hit", false);
    bool wasBlocking = mapping->GetValueAsBool("p1_was_blocking", false);
    int framesSinceLastHit = mapping->GetValueAsInt("p1_frames_since_hit", 0);
    
    // Reversal if player was hit or blocking very recently (within 10 frames)
    return (wasHit || wasBlocking) && framesSinceLastHit < 10;
}

bool AIComboClassifier::IsCounterHit(const AIMemoryMapping* mapping) const {
    if (!mapping) return false;
    
    return mapping->GetValueAsBool("p2_counter_hit", false);
}

float AIComboClassifier::CalculateDiversityScore() const {
    if (m_comboTypeCount.empty()) {
        return 0.0f;
    }
    
    // Shannon entropy as diversity metric
    float entropy = 0.0f;
    int totalCombos = 0;
    
    for (const auto& pair : m_comboTypeCount) {
        totalCombos += pair.second;
    }
    
    for (const auto& pair : m_comboTypeCount) {
        float probability = static_cast<float>(pair.second) / static_cast<float>(totalCombos);
        entropy -= probability * std::log2(probability);
    }
    
    // Normalize by max possible entropy (if all types equally likely)
    float maxEntropy = std::log2(static_cast<float>(m_comboTypeCount.size()));
    if (maxEntropy > 0) {
        return entropy / maxEntropy;
    }
    
    return 0.0f;
}

// Helper for ending the current combo and creating a ComboEvent
AIComboClassifier::ComboEvent AIComboClassifier::EndCurrentCombo(int endFrame) {
    ComboEvent event;
    event.frameStart = m_comboStartFrame;
    event.frameEnd = endFrame;
    event.hitCount = m_hitCounter;
    event.damage = m_totalDamage;
    event.actions = m_comboActions;
    event.complexity = CalculateComplexity(m_comboActions);
    
    // Generate a simple description based on hit count and damage
    std::string desc;
    if (m_hitCounter >= 10) {
        desc = "Massive ";
    } else if (m_hitCounter >= 7) {
        desc = "Big ";
    } else if (m_hitCounter >= 4) {
        desc = "Medium ";
    } else {
        desc = "Small ";
    }
    
    if (m_totalDamage >= 70) {
        desc += "Devastation";
    } else if (m_totalDamage >= 50) {
        desc += "Destroyer";
    } else if (m_totalDamage >= 30) {
        desc += "Punisher";
    } else {
        desc += "Combo";
    }
    
    event.description = desc;
    
    // Record combo type for diversity scoring
    m_comboTypeCount[desc]++;
    
    return event;
} 