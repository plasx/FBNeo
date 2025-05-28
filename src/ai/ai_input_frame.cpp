#include "ai_input_frame.h"
#include "ai_memory_mapping.h"
#include <nlohmann/json.hpp>
#include <xxhash.h>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <functional>
#include <iomanip>
#include <iostream>
#include "../burner/burner.h"

// Forward declaration - will be defined in ai_memory_mapping.h
// #include "ai_memory_mapping.h"

using json = nlohmann::json;

namespace AI {

AIInputFrame::AIInputFrame()
    : m_frameNumber(0)
    , m_gameId("")
    , m_hash("")
    , frameNumber(0)
    , gameTime(0.0f)
    , playerValues{}
    , distanceBetweenPlayers(0.0f)
    , horizontalDistance(0.0f)
    , verticalDistance(0.0f)
    , playerWon(-1)
    , roundEnded(false)
    , p1Health(0)
    , p2Health(0)
    , p1Meter(0)
    , p2Meter(0)
{
    // Initialize all values to defaults
    time_remaining = 0.0f;
    round = 0.0f;
    
    p1_x = 0.5f;
    p1_y = 0.0f;
    p1_health = 1.0f;
    p1_meter = 0.0f;
    p1_state = 0.0f;
    p1_facing = 1.0f;
    p1_attacking = 0.0f;
    p1_blocking = 0.0f;
    
    p2_x = 0.5f;
    p2_y = 0.0f;
    p2_health = 1.0f;
    p2_meter = 0.0f;
    p2_state = 0.0f;
    p2_facing = -1.0f;
    p2_attacking = 0.0f;
    p2_blocking = 0.0f;
    
    x_distance = 0.3f;
    y_distance = 0.0f;
}

AIInputFrame::AIInputFrame(int frameNumber)
    : m_frameNumber(frameNumber)
    , m_gameId("")
    , m_hash("")
    , frameNumber(frameNumber)
    , gameTime(0.0f)
    , playerValues{}
    , distanceBetweenPlayers(0.0f)
    , horizontalDistance(0.0f)
    , verticalDistance(0.0f)
    , playerWon(-1)
    , roundEnded(false)
    , p1Health(0)
    , p2Health(0)
    , p1Meter(0)
    , p2Meter(0)
{
    // Initialize with default values
    time_remaining = 0.0f;
    round = 0.0f;
    
    p1_x = 0.5f;
    p1_y = 0.0f;
    p1_health = 1.0f;
    p1_meter = 0.0f;
    p1_state = 0.0f;
    p1_facing = 1.0f;
    p1_attacking = 0.0f;
    p1_blocking = 0.0f;
    
    p2_x = 0.5f;
    p2_y = 0.0f;
    p2_health = 1.0f;
    p2_meter = 0.0f;
    p2_state = 0.0f;
    p2_facing = -1.0f;
    p2_attacking = 0.0f;
    p2_blocking = 0.0f;
    
    x_distance = 0.3f;
    y_distance = 0.0f;
}

AIInputFrame::~AIInputFrame()
{
    // Nothing to do here
}

void AIInputFrame::setFrameNumber(int frameNumber)
{
    m_frameNumber = frameNumber;
}

int AIInputFrame::getFrameNumber() const
{
    return m_frameNumber;
}

void AIInputFrame::setGameId(const std::string& gameId)
{
    m_gameId = gameId;
}

const std::string& AIInputFrame::getGameId() const
{
    return m_gameId;
}

void AIInputFrame::setHash(const std::string& hash)
{
    m_hash = hash;
}

const std::string& AIInputFrame::getHash() const
{
    return m_hash;
}

void AIInputFrame::generateHash()
{
    // Simple hash generation using string concatenation for testing
    std::stringstream ss;
    
    ss << m_frameNumber << m_gameId;
    
    // Add player values
    for (const auto& playerPair : m_playerValues) {
        ss << "p" << playerPair.first;
        
        // Sort the keys for deterministic ordering
        std::vector<std::string> keys;
        for (const auto& valuePair : playerPair.second) {
            keys.push_back(valuePair.first);
        }
        std::sort(keys.begin(), keys.end());
        
        for (const auto& key : keys) {
            ss << key << playerPair.second.at(key);
        }
    }
    
    // Add feature values
    std::vector<std::string> featureKeys;
    for (const auto& valuePair : m_featureValues) {
        featureKeys.push_back(valuePair.first);
    }
    std::sort(featureKeys.begin(), featureKeys.end());
    
    for (const auto& key : featureKeys) {
        ss << key << m_featureValues.at(key);
    }
    
    // Create a simple hash
    std::hash<std::string> hasher;
    std::size_t hashValue = hasher(ss.str());
    
    // Convert to hex string
    std::stringstream hashStream;
    hashStream << std::hex << std::setfill('0') << std::setw(16) << hashValue;
    m_hash = hashStream.str();
}

void AIInputFrame::addPlayerValue(int playerIndex, const std::string& name, float value)
{
    m_playerValues[playerIndex][name] = value;
}

float AIInputFrame::getPlayerValue(int playerIndex, const std::string& name) const
{
    auto playerIt = m_playerValues.find(playerIndex);
    if (playerIt != m_playerValues.end()) {
        auto valueIt = playerIt->second.find(name);
        if (valueIt != playerIt->second.end()) {
            return valueIt->second;
        }
    }
    return 0.0f;
}

std::vector<std::string> AIInputFrame::getPlayerValueNames(int playerIndex) const
{
    std::vector<std::string> result;
    
    auto playerIt = m_playerValues.find(playerIndex);
    if (playerIt != m_playerValues.end()) {
        for (const auto& valuePair : playerIt->second) {
            result.push_back(valuePair.first);
        }
    }
    
    return result;
}

std::vector<int> AIInputFrame::getPlayerIndices() const
{
    std::vector<int> result;
    
    for (const auto& playerPair : m_playerValues) {
        result.push_back(playerPair.first);
    }
    
    return result;
}

void AIInputFrame::addFeatureValue(const std::string& name, float value)
{
    m_featureValues[name] = value;
}

float AIInputFrame::getFeatureValue(const std::string& name) const
{
    auto it = m_featureValues.find(name);
    if (it != m_featureValues.end()) {
        return it->second;
    }
    return 0.0f;
}

std::vector<std::string> AIInputFrame::getFeatureValueNames() const
{
    std::vector<std::string> result;
    
    for (const auto& valuePair : m_featureValues) {
        result.push_back(valuePair.first);
    }
    
    return result;
}

std::string AIInputFrame::toJson() const
{
    // Simplified implementation for testing
    std::stringstream ss;
    
    ss << "{";
    ss << "\"frame_number\":" << m_frameNumber << ",";
    ss << "\"game_id\":\"" << m_gameId << "\",";
    
    if (!m_hash.empty()) {
        ss << "\"hash\":\"" << m_hash << "\",";
    }
    
    // Standard attributes
    ss << "\"time_remaining\":" << time_remaining << ",";
    ss << "\"round\":" << round << ",";
    
    // Player 1
    ss << "\"p1\":{";
    ss << "\"x\":" << p1_x << ",";
    ss << "\"y\":" << p1_y << ",";
    ss << "\"health\":" << p1_health << ",";
    ss << "\"meter\":" << p1_meter << ",";
    ss << "\"state\":" << p1_state << ",";
    ss << "\"facing\":" << p1_facing << ",";
    ss << "\"attacking\":" << p1_attacking << ",";
    ss << "\"blocking\":" << p1_blocking;
    ss << "},";
    
    // Player 2
    ss << "\"p2\":{";
    ss << "\"x\":" << p2_x << ",";
    ss << "\"y\":" << p2_y << ",";
    ss << "\"health\":" << p2_health << ",";
    ss << "\"meter\":" << p2_meter << ",";
    ss << "\"state\":" << p2_state << ",";
    ss << "\"facing\":" << p2_facing << ",";
    ss << "\"attacking\":" << p2_attacking << ",";
    ss << "\"blocking\":" << p2_blocking;
    ss << "},";
    
    // Distance metrics
    ss << "\"distance\":{";
    ss << "\"x\":" << x_distance << ",";
    ss << "\"y\":" << y_distance;
    ss << "}";
    
    ss << "}";
    
    return ss.str();
}

bool AIInputFrame::fromJson(const std::string& jsonString)
{
    // Simplified implementation for testing
    // A real implementation would use nlohmann/json
    return false;
}

bool AIInputFrame::equals(const AIInputFrame& other) const
{
    return m_frameNumber == other.m_frameNumber &&
           m_gameId == other.m_gameId &&
           m_hash == other.m_hash &&
           std::abs(time_remaining - other.time_remaining) < 0.0001f &&
           std::abs(round - other.round) < 0.0001f &&
           std::abs(p1_x - other.p1_x) < 0.0001f &&
           std::abs(p1_y - other.p1_y) < 0.0001f &&
           std::abs(p1_health - other.p1_health) < 0.0001f &&
           std::abs(p1_meter - other.p1_meter) < 0.0001f &&
           std::abs(p1_state - other.p1_state) < 0.0001f &&
           std::abs(p1_facing - other.p1_facing) < 0.0001f &&
           std::abs(p1_attacking - other.p1_attacking) < 0.0001f &&
           std::abs(p1_blocking - other.p1_blocking) < 0.0001f &&
           std::abs(p2_x - other.p2_x) < 0.0001f &&
           std::abs(p2_y - other.p2_y) < 0.0001f &&
           std::abs(p2_health - other.p2_health) < 0.0001f &&
           std::abs(p2_meter - other.p2_meter) < 0.0001f &&
           std::abs(p2_state - other.p2_state) < 0.0001f &&
           std::abs(p2_facing - other.p2_facing) < 0.0001f &&
           std::abs(p2_attacking - other.p2_attacking) < 0.0001f &&
           std::abs(p2_blocking - other.p2_blocking) < 0.0001f &&
           std::abs(x_distance - other.x_distance) < 0.0001f &&
           std::abs(y_distance - other.y_distance) < 0.0001f;
}

bool AIInputFrame::findDifferences(const AIInputFrame& other, std::vector<std::string>& differences) const
{
    differences.clear();
    
    if (m_frameNumber != other.m_frameNumber) {
        differences.push_back("frame_number");
    }
    
    if (m_gameId != other.m_gameId) {
        differences.push_back("game_id");
    }
    
    if (std::abs(time_remaining - other.time_remaining) > 0.0001f) {
        differences.push_back("time_remaining");
    }
    
    if (std::abs(round - other.round) > 0.0001f) {
        differences.push_back("round");
    }
    
    // Player 1
    if (std::abs(p1_x - other.p1_x) > 0.0001f) {
        differences.push_back("p1_x");
    }
    
    if (std::abs(p1_y - other.p1_y) > 0.0001f) {
        differences.push_back("p1_y");
    }
    
    if (std::abs(p1_health - other.p1_health) > 0.0001f) {
        differences.push_back("p1_health");
    }
    
    if (std::abs(p1_meter - other.p1_meter) > 0.0001f) {
        differences.push_back("p1_meter");
    }
    
    if (std::abs(p1_state - other.p1_state) > 0.0001f) {
        differences.push_back("p1_state");
    }
    
    if (std::abs(p1_facing - other.p1_facing) > 0.0001f) {
        differences.push_back("p1_facing");
    }
    
    if (std::abs(p1_attacking - other.p1_attacking) > 0.0001f) {
        differences.push_back("p1_attacking");
    }
    
    if (std::abs(p1_blocking - other.p1_blocking) > 0.0001f) {
        differences.push_back("p1_blocking");
    }
    
    // Player 2
    if (std::abs(p2_x - other.p2_x) > 0.0001f) {
        differences.push_back("p2_x");
    }
    
    if (std::abs(p2_y - other.p2_y) > 0.0001f) {
        differences.push_back("p2_y");
    }
    
    if (std::abs(p2_health - other.p2_health) > 0.0001f) {
        differences.push_back("p2_health");
    }
    
    if (std::abs(p2_meter - other.p2_meter) > 0.0001f) {
        differences.push_back("p2_meter");
    }
    
    if (std::abs(p2_state - other.p2_state) > 0.0001f) {
        differences.push_back("p2_state");
    }
    
    if (std::abs(p2_facing - other.p2_facing) > 0.0001f) {
        differences.push_back("p2_facing");
    }
    
    if (std::abs(p2_attacking - other.p2_attacking) > 0.0001f) {
        differences.push_back("p2_attacking");
    }
    
    if (std::abs(p2_blocking - other.p2_blocking) > 0.0001f) {
        differences.push_back("p2_blocking");
    }
    
    // Distance metrics
    if (std::abs(x_distance - other.x_distance) > 0.0001f) {
        differences.push_back("x_distance");
    }
    
    if (std::abs(y_distance - other.y_distance) > 0.0001f) {
        differences.push_back("y_distance");
    }
    
    return !differences.empty();
}

std::string AIInputFrame::toString() const
{
    std::stringstream ss;
    
    ss << "Frame: " << m_frameNumber;
    if (!m_gameId.empty()) {
        ss << ", Game: " << m_gameId;
    }
    
    ss << ", Time: " << time_remaining;
    ss << ", Round: " << round;
    
    ss << " | P1: ";
    ss << "health=" << p1_health;
    ss << ", meter=" << p1_meter;
    ss << ", pos=(" << p1_x << "," << p1_y << ")";
    ss << ", facing=" << (p1_facing > 0 ? "right" : "left");
    
    ss << " | P2: ";
    ss << "health=" << p2_health;
    ss << ", meter=" << p2_meter;
    ss << ", pos=(" << p2_x << "," << p2_y << ")";
    ss << ", facing=" << (p2_facing > 0 ? "right" : "left");
    
    ss << " | Distance: (" << x_distance << "," << y_distance << ")";
    
    return ss.str();
}

AIInputFrame::AIInputFrame(const AIInputFrame& other) 
    : time_remaining(other.time_remaining), round(other.round),
      p1_x(other.p1_x), p1_y(other.p1_y), p1_health(other.p1_health), p1_meter(other.p1_meter),
      p1_state(other.p1_state), p1_facing(other.p1_facing), p1_attacking(other.p1_attacking), p1_blocking(other.p1_blocking),
      p2_x(other.p2_x), p2_y(other.p2_y), p2_health(other.p2_health), p2_meter(other.p2_meter),
      p2_state(other.p2_state), p2_facing(other.p2_facing), p2_attacking(other.p2_attacking), p2_blocking(other.p2_blocking),
      x_distance(other.x_distance), y_distance(other.y_distance),
      frame_number(other.frame_number), inputs(other.inputs), rng_seed(other.rng_seed), state_hash(other.state_hash),
      frameNumber(other.frameNumber), gameTime(other.gameTime), playerValues(other.playerValues),
      distanceBetweenPlayers(other.distanceBetweenPlayers), horizontalDistance(other.horizontalDistance),
      verticalDistance(other.verticalDistance), playerWon(other.playerWon), roundEnded(other.roundEnded),
      p1Health(other.p1Health), p2Health(other.p2Health), p1Meter(other.p1Meter), p2Meter(other.p2Meter) {
}

AIInputFrame AIInputFrame::extractFromMemory(const AIMemoryMapping& mapping) {
    AIInputFrame frame;
    
    // Extract values based on mapping
    try {
        // Match state
        frame.time_remaining = mapping.readMemoryValue("time_remaining");
        frame.round = mapping.readMemoryValue("round");
        
        // Player 1 state
        frame.p1_x = mapping.readMemoryValue("p1_x");
        frame.p1_y = mapping.readMemoryValue("p1_y");
        frame.p1_health = mapping.readMemoryValue("p1_health");
        frame.p1_meter = mapping.readMemoryValue("p1_meter");
        frame.p1_state = mapping.readMemoryValue("p1_state");
        frame.p1_facing = mapping.readMemoryValue("p1_facing");
        frame.p1_attacking = mapping.readMemoryValue("p1_attacking");
        frame.p1_blocking = mapping.readMemoryValue("p1_blocking");
        
        // Player 2 state
        frame.p2_x = mapping.readMemoryValue("p2_x");
        frame.p2_y = mapping.readMemoryValue("p2_y");
        frame.p2_health = mapping.readMemoryValue("p2_health");
        frame.p2_meter = mapping.readMemoryValue("p2_meter");
        frame.p2_state = mapping.readMemoryValue("p2_state");
        frame.p2_facing = mapping.readMemoryValue("p2_facing");
        frame.p2_attacking = mapping.readMemoryValue("p2_attacking");
        frame.p2_blocking = mapping.readMemoryValue("p2_blocking");
        
        // Calculate derived values
        float p1x = frame.p1_x;
        float p2x = frame.p2_x;
        float p1y = frame.p1_y;
        float p2y = frame.p2_y;
        
        // Calculate distances
        frame.x_distance = std::abs(p1x - p2x);
        frame.y_distance = std::abs(p1y - p2y);
        
        // Apply normalization
        frame.normalize();
    }
    catch (const std::exception& e) {
        // Log error and return default frame
        printf("Error extracting frame data: %s\n", e.what());
    }
    
    return frame;
}

void AIInputFrame::normalize() {
    // Clamp all values to their expected ranges
    time_remaining = std::max(0.0f, std::min(1.0f, time_remaining));
    round = std::max(0.0f, std::min(1.0f, round));
    
    p1_health = std::max(0.0f, std::min(1.0f, p1_health));
    p1_meter = std::max(0.0f, std::min(1.0f, p1_meter));
    p1_state = std::max(0.0f, std::min(10.0f, p1_state)) / 10.0f; // Assuming max 10 states
    p1_attacking = p1_attacking > 0.5f ? 1.0f : 0.0f; // Boolean
    p1_blocking = p1_blocking > 0.5f ? 1.0f : 0.0f; // Boolean
    
    p2_health = std::max(0.0f, std::min(1.0f, p2_health));
    p2_meter = std::max(0.0f, std::min(1.0f, p2_meter));
    p2_state = std::max(0.0f, std::min(10.0f, p2_state)) / 10.0f; // Assuming max 10 states
    p2_attacking = p2_attacking > 0.5f ? 1.0f : 0.0f; // Boolean
    p2_blocking = p2_blocking > 0.5f ? 1.0f : 0.0f; // Boolean
    
    // Normalize positions to [0.0, 1.0] range
    // This assumes the game's coordinate system is already known
    p1_x = std::max(0.0f, std::min(1.0f, p1_x));
    p1_y = std::max(0.0f, std::min(1.0f, p1_y));
    p2_x = std::max(0.0f, std::min(1.0f, p2_x));
    p2_y = std::max(0.0f, std::min(1.0f, p2_y));
    
    // Normalize distances
    x_distance = std::max(0.0f, std::min(1.0f, x_distance));
    y_distance = std::max(0.0f, std::min(1.0f, y_distance));
    
    // Make sure facing directions are binary
    p1_facing = p1_facing >= 0.0f ? 1.0f : -1.0f;
    p2_facing = p2_facing >= 0.0f ? 1.0f : -1.0f;
}

std::vector<float> AIInputFrame::toVector() const {
    // Create a flat vector of all features for model input
    std::vector<float> vec = {
        time_remaining,
        round,
        
        p1_x,
        p1_y,
        p1_health,
        p1_meter,
        p1_state,
        p1_facing,
        p1_attacking,
        p1_blocking,
        
        p2_x,
        p2_y,
        p2_health,
        p2_meter,
        p2_state,
        p2_facing,
        p2_attacking,
        p2_blocking,
        
        x_distance,
        y_distance
    };
    
    return vec;
}

std::string AIInputFrame::toJSON() const {
    json j;
    
    // Match state
    j["time_remaining"] = time_remaining;
    j["round"] = round;
    
    // Player 1 state
    j["p1_x"] = p1_x;
    j["p1_y"] = p1_y;
    j["p1_health"] = p1_health;
    j["p1_meter"] = p1_meter;
    j["p1_state"] = p1_state;
    j["p1_facing"] = p1_facing;
    j["p1_attacking"] = p1_attacking;
    j["p1_blocking"] = p1_blocking;
    
    // Player 2 state
    j["p2_x"] = p2_x;
    j["p2_y"] = p2_y;
    j["p2_health"] = p2_health;
    j["p2_meter"] = p2_meter;
    j["p2_state"] = p2_state;
    j["p2_facing"] = p2_facing;
    j["p2_attacking"] = p2_attacking;
    j["p2_blocking"] = p2_blocking;
    
    // Distance metrics
    j["x_distance"] = x_distance;
    j["y_distance"] = y_distance;
    
    // Metadata
    j["frame"] = frame_number;
    j["inputs"] = inputs;
    j["rng_seed"] = rng_seed;
    j["state_hash"] = state_hash;
    
    return j.dump();
}

AIInputFrame AIInputFrame::fromJSON(const std::string& jsonStr) {
    AIInputFrame frame;
    json j = json::parse(jsonStr);
    
    // Match state
    frame.time_remaining = j["time_remaining"];
    frame.round = j["round"];
    
    // Player 1 state
    frame.p1_x = j["p1_x"];
    frame.p1_y = j["p1_y"];
    frame.p1_health = j["p1_health"];
    frame.p1_meter = j["p1_meter"];
    frame.p1_state = j["p1_state"];
    frame.p1_facing = j["p1_facing"];
    frame.p1_attacking = j["p1_attacking"];
    frame.p1_blocking = j["p1_blocking"];
    
    // Player 2 state
    frame.p2_x = j["p2_x"];
    frame.p2_y = j["p2_y"];
    frame.p2_health = j["p2_health"];
    frame.p2_meter = j["p2_meter"];
    frame.p2_state = j["p2_state"];
    frame.p2_facing = j["p2_facing"];
    frame.p2_attacking = j["p2_attacking"];
    frame.p2_blocking = j["p2_blocking"];
    
    // Distance metrics
    frame.x_distance = j["x_distance"];
    frame.y_distance = j["y_distance"];
    
    // Metadata
    frame.frame_number = j["frame"];
    frame.inputs = j["inputs"];
    frame.rng_seed = j["rng_seed"];
    frame.state_hash = j["state_hash"];
    
    return frame;
}

void AIInputFrame::computeStateHash() {
    // Use xxHash for fast hashing
    XXH64_state_t* state = XXH64_createState();
    XXH64_reset(state, 0); // Use a fixed seed (0) for determinism
    
    // Hash all important state data
    XXH64_update(state, &time_remaining, sizeof(time_remaining));
    XXH64_update(state, &round, sizeof(round));
    
    XXH64_update(state, &p1_x, sizeof(p1_x));
    XXH64_update(state, &p1_y, sizeof(p1_y));
    XXH64_update(state, &p1_health, sizeof(p1_health));
    XXH64_update(state, &p1_meter, sizeof(p1_meter));
    XXH64_update(state, &p1_state, sizeof(p1_state));
    XXH64_update(state, &p1_facing, sizeof(p1_facing));
    XXH64_update(state, &p1_attacking, sizeof(p1_attacking));
    XXH64_update(state, &p1_blocking, sizeof(p1_blocking));
    
    XXH64_update(state, &p2_x, sizeof(p2_x));
    XXH64_update(state, &p2_y, sizeof(p2_y));
    XXH64_update(state, &p2_health, sizeof(p2_health));
    XXH64_update(state, &p2_meter, sizeof(p2_meter));
    XXH64_update(state, &p2_state, sizeof(p2_state));
    XXH64_update(state, &p2_facing, sizeof(p2_facing));
    XXH64_update(state, &p2_attacking, sizeof(p2_attacking));
    XXH64_update(state, &p2_blocking, sizeof(p2_blocking));
    
    XXH64_update(state, &rng_seed, sizeof(rng_seed));
    
    // Get the hash
    uint64_t hash = XXH64_digest(state);
    
    // Convert to string
    std::stringstream ss;
    ss << std::hex << hash;
    state_hash = ss.str();
    
    // Free the state
    XXH64_freeState(state);
}

bool AIInputFrame::operator==(const AIInputFrame& other) const {
    // State hash should capture all important state
    if (!state_hash.empty() && !other.state_hash.empty()) {
        return state_hash == other.state_hash;
    }
    
    // If hashes aren't available, do a field-by-field comparison
    return 
        frame_number == other.frame_number &&
        std::abs(time_remaining - other.time_remaining) < 0.0001f &&
        std::abs(round - other.round) < 0.0001f &&
        
        std::abs(p1_x - other.p1_x) < 0.0001f &&
        std::abs(p1_y - other.p1_y) < 0.0001f &&
        std::abs(p1_health - other.p1_health) < 0.0001f &&
        std::abs(p1_meter - other.p1_meter) < 0.0001f &&
        std::abs(p1_state - other.p1_state) < 0.0001f &&
        p1_facing == other.p1_facing &&
        p1_attacking == other.p1_attacking &&
        p1_blocking == other.p1_blocking &&
        
        std::abs(p2_x - other.p2_x) < 0.0001f &&
        std::abs(p2_y - other.p2_y) < 0.0001f &&
        std::abs(p2_health - other.p2_health) < 0.0001f &&
        std::abs(p2_meter - other.p2_meter) < 0.0001f &&
        std::abs(p2_state - other.p2_state) < 0.0001f &&
        p2_facing == other.p2_facing &&
        p2_attacking == other.p2_attacking &&
        p2_blocking == other.p2_blocking &&
        
        rng_seed == other.rng_seed;
}

bool AIInputFrame::operator!=(const AIInputFrame& other) const {
    return !(*this == other);
}

size_t AIInputFrame::getInputDimension() {
    // Return the number of input features
    // This should match the size of the vector returned by toVector()
    return 20; // Update if you add or remove features
}

// Constructor
AIInputFrame::AIInputFrame(int player_idx)
    : m_player_index(player_idx),
      m_frame_counter(0),
      m_health(1.0f),
      m_opponent_health(1.0f),
      m_position_x(0.5f),
      m_position_y(0.5f),
      m_opponent_position_x(0.5f),
      m_opponent_position_y(0.5f),
      m_memory_mapping(nullptr) {
}

// Destructor
AIInputFrame::~AIInputFrame()
{
    // We don't own the memory mapping, so don't delete it
}

// Initialize the frame with the current game state
void AIInputFrame::Init(AIMemoryMapping* memory_mapping)
{
    SetMemoryMapping(memory_mapping);
    Update();
}

// Set the memory mapping to use for game-specific values
void AIInputFrame::SetMemoryMapping(AIMemoryMapping* memory_mapping)
{
    m_memory_mapping = memory_mapping;
}

// Update the frame with the current game state
void AIInputFrame::Update()
{
    if (m_memory_mapping) {
        // Update standard features from memory mappings if available
        UpdateStandardFeaturesFromMapping();
        
        // Update game-specific memory values
        UpdateGameSpecificValues();
    }
}

// Update standard features from memory mappings
void AIInputFrame::UpdateStandardFeaturesFromMapping()
{
    // We need to determine the player prefix based on player index
    std::string player_prefix = (m_player_index == 1) ? "p1_" : "p2_";
    std::string opponent_prefix = (m_player_index == 1) ? "p2_" : "p1_";
    
    // Health
    if (m_memory_mapping->HasMappingForGame(m_memory_mapping->GetLoadedGame())) {
        // Check for health mappings
        std::string health_mapping = player_prefix + "health";
        std::string opponent_health_mapping = opponent_prefix + "health";
        
        // Update health values if mappings exist
        if (auto mapping = m_memory_mapping->GetMapping(health_mapping)) {
            // Use normalized value if min/max are available, otherwise use raw value
            if (mapping->min_value.has_value() && mapping->max_value.has_value()) {
                m_health = m_memory_mapping->ReadNormalizedValue(health_mapping);
            } else {
                // Try to normalize based on common health values
                float raw_health = m_memory_mapping->ReadValue(health_mapping);
                if (raw_health <= 100.0f) {
                    m_health = raw_health / 100.0f;
                } else if (raw_health <= 255.0f) {
                    m_health = raw_health / 255.0f;
                } else {
                    m_health = 1.0f; // Default to full health
                }
            }
        }
        
        // Update opponent health value if mapping exists
        if (auto mapping = m_memory_mapping->GetMapping(opponent_health_mapping)) {
            // Use normalized value if min/max are available, otherwise use raw value
            if (mapping->min_value.has_value() && mapping->max_value.has_value()) {
                m_opponent_health = m_memory_mapping->ReadNormalizedValue(opponent_health_mapping);
            } else {
                // Try to normalize based on common health values
                float raw_health = m_memory_mapping->ReadValue(opponent_health_mapping);
                if (raw_health <= 100.0f) {
                    m_opponent_health = raw_health / 100.0f;
                } else if (raw_health <= 255.0f) {
                    m_opponent_health = raw_health / 255.0f;
                } else {
                    m_opponent_health = 1.0f; // Default to full health
                }
            }
        }
        
        // Check for position mappings
        std::string pos_x_mapping = player_prefix + "x_position";
        std::string pos_y_mapping = player_prefix + "y_position";
        std::string opponent_pos_x_mapping = opponent_prefix + "x_position";
        std::string opponent_pos_y_mapping = opponent_prefix + "y_position";
        
        // Update position values if mappings exist
        if (auto mapping = m_memory_mapping->GetMapping(pos_x_mapping)) {
            // Use normalized value if min/max are available, otherwise estimate
            if (mapping->min_value.has_value() && mapping->max_value.has_value()) {
                m_position_x = m_memory_mapping->ReadNormalizedValue(pos_x_mapping);
            } else {
                // Attempt to normalize based on screen width (very game-specific)
                float raw_pos_x = m_memory_mapping->ReadValue(pos_x_mapping);
                // Assuming a typical screen width of 320-384 pixels
                m_position_x = std::max(0.0f, std::min(1.0f, raw_pos_x / 384.0f));
            }
        }
        
        if (auto mapping = m_memory_mapping->GetMapping(pos_y_mapping)) {
            // Use normalized value if min/max are available, otherwise estimate
            if (mapping->min_value.has_value() && mapping->max_value.has_value()) {
                m_position_y = m_memory_mapping->ReadNormalizedValue(pos_y_mapping);
            } else {
                // Attempt to normalize based on screen height (very game-specific)
                float raw_pos_y = m_memory_mapping->ReadValue(pos_y_mapping);
                // Assuming a typical screen height of 224-256 pixels
                m_position_y = std::max(0.0f, std::min(1.0f, raw_pos_y / 256.0f));
            }
        }
        
        if (auto mapping = m_memory_mapping->GetMapping(opponent_pos_x_mapping)) {
            // Use normalized value if min/max are available, otherwise estimate
            if (mapping->min_value.has_value() && mapping->max_value.has_value()) {
                m_opponent_position_x = m_memory_mapping->ReadNormalizedValue(opponent_pos_x_mapping);
            } else {
                // Attempt to normalize based on screen width
                float raw_pos_x = m_memory_mapping->ReadValue(opponent_pos_x_mapping);
                m_opponent_position_x = std::max(0.0f, std::min(1.0f, raw_pos_x / 384.0f));
            }
        }
        
        if (auto mapping = m_memory_mapping->GetMapping(opponent_pos_y_mapping)) {
            // Use normalized value if min/max are available, otherwise estimate
            if (mapping->min_value.has_value() && mapping->max_value.has_value()) {
                m_opponent_position_y = m_memory_mapping->ReadNormalizedValue(opponent_pos_y_mapping);
            } else {
                // Attempt to normalize based on screen height
                float raw_pos_y = m_memory_mapping->ReadValue(opponent_pos_y_mapping);
                m_opponent_position_y = std::max(0.0f, std::min(1.0f, raw_pos_y / 256.0f));
            }
        }
    }
}

// Update game-specific memory values
void AIInputFrame::UpdateGameSpecificValues()
{
    if (!m_memory_mapping) {
        return;
    }
    
    // Clear previous values
    m_memory_values.clear();
    
    // Get all mapping names
    std::vector<std::string> mapping_names = m_memory_mapping->GetMappingNames();
    
    // Read all values and store them
    for (const auto& name : mapping_names) {
        // Skip standard mappings that we already have
        if (name.find("health") != std::string::npos ||
            name.find("position") != std::string::npos) {
            continue;
        }
        
        // Read the value and store it
        try {
            float value = m_memory_mapping->ReadNormalizedValue(name);
            m_memory_values[name] = value;
        } catch (const std::exception& e) {
            // In case of any errors, just skip this value
            std::cerr << "Error reading memory value " << name << ": " << e.what() << std::endl;
        }
    }
}

// Get the player index
int AIInputFrame::GetPlayerIndex() const
{
    return m_player_index;
}

// Set the player index
void AIInputFrame::SetPlayerIndex(int idx)
{
    m_player_index = idx;
}

// Get the frame counter
int AIInputFrame::GetFrameCounter() const
{
    return m_frame_counter;
}

// Set the frame counter
void AIInputFrame::SetFrameCounter(int counter)
{
    m_frame_counter = counter;
}

// Get the normalized health value
float AIInputFrame::GetHealth() const
{
    return m_health;
}

// Set the normalized health value
void AIInputFrame::SetHealth(float health)
{
    m_health = std::max(0.0f, std::min(1.0f, health));
}

// Get the opponent's normalized health value
float AIInputFrame::GetOpponentHealth() const
{
    return m_opponent_health;
}

// Set the opponent's normalized health value
void AIInputFrame::SetOpponentHealth(float health)
{
    m_opponent_health = std::max(0.0f, std::min(1.0f, health));
}

// Get the normalized X position
float AIInputFrame::GetPositionX() const
{
    return m_position_x;
}

// Set the normalized X position
void AIInputFrame::SetPositionX(float x)
{
    m_position_x = std::max(0.0f, std::min(1.0f, x));
}

// Get the normalized Y position
float AIInputFrame::GetPositionY() const
{
    return m_position_y;
}

// Set the normalized Y position
void AIInputFrame::SetPositionY(float y)
{
    m_position_y = std::max(0.0f, std::min(1.0f, y));
}

// Get the opponent's normalized X position
float AIInputFrame::GetOpponentPositionX() const
{
    return m_opponent_position_x;
}

// Set the opponent's normalized X position
void AIInputFrame::SetOpponentPositionX(float x)
{
    m_opponent_position_x = std::max(0.0f, std::min(1.0f, x));
}

// Get the opponent's normalized Y position
float AIInputFrame::GetOpponentPositionY() const
{
    return m_opponent_position_y;
}

// Set the opponent's normalized Y position
void AIInputFrame::SetOpponentPositionY(float y)
{
    m_opponent_position_y = std::max(0.0f, std::min(1.0f, y));
}

// Get the normalized distance to the opponent
float AIInputFrame::GetDistanceToOpponent() const
{
    // Calculate Euclidean distance
    float dx = m_position_x - m_opponent_position_x;
    float dy = m_position_y - m_opponent_position_y;
    
    // Normalize by max possible distance (diagonal of 1x1 square = sqrt(2))
    return std::min(1.0f, std::sqrt(dx * dx + dy * dy) / std::sqrt(2.0f));
}

// Set a game-specific memory value
void AIInputFrame::SetMemoryValue(const std::string& name, float value)
{
    m_memory_values[name] = value;
}

// Get a game-specific memory value
float AIInputFrame::GetMemoryValue(const std::string& name) const
{
    auto it = m_memory_values.find(name);
    if (it != m_memory_values.end()) {
        return it->second;
    }
    return 0.0f;
}

// Check if a game-specific memory value exists
bool AIInputFrame::HasMemoryValue(const std::string& name) const
{
    return m_memory_values.find(name) != m_memory_values.end();
}

// Get all game-specific memory values
const std::unordered_map<std::string, float>& AIInputFrame::GetMemoryValues() const
{
    return m_memory_values;
}

// Get a vector of all values (standard + game-specific)
std::vector<float> AIInputFrame::GetFeatureVector() const {
    std::vector<float> features;
    
    // Reserve space for all features to avoid reallocations
    features.reserve(30);
    
    // Game state features
    features.push_back(static_cast<float>(frameNumber) / 3600.0f);  // Normalize frame number (assuming 60fps for 60 seconds)
    features.push_back(gameTime / 99.0f);  // Normalize game time (assuming max 99 seconds)
    
    // Player 1 features
    features.push_back(p1_x / 400.0f);  // Normalize x position (assuming screen width of 400)
    features.push_back(p1_y / 300.0f);  // Normalize y position (assuming screen height of 300)
    features.push_back(static_cast<float>(p1_state) / 10.0f);  // Normalize state (assuming 10 states)
    features.push_back(p1_facing == 1.0f ? 1.0f : 0.0f);
    
    // Player 2 features
    features.push_back(p2_x / 400.0f);
    features.push_back(p2_y / 300.0f);
    features.push_back(static_cast<float>(p2_state) / 10.0f);
    features.push_back(p2_facing == 1.0f ? 1.0f : 0.0f);
    
    // Distance features
    features.push_back(distanceBetweenPlayers / 400.0f);
    features.push_back(horizontalDistance / 400.0f);
    features.push_back(verticalDistance / 300.0f);
    
    // Health and meter
    features.push_back(static_cast<float>(p1Health) / 100.0f);
    features.push_back(static_cast<float>(p2Health) / 100.0f);
    features.push_back(static_cast<float>(p1Meter) / 100.0f);
    features.push_back(static_cast<float>(p2Meter) / 100.0f);
    
    // Round state
    features.push_back(roundEnded ? 1.0f : 0.0f);
    features.push_back((playerWon == 0) ? 1.0f : 0.0f);
    features.push_back((playerWon == 1) ? 1.0f : 0.0f);
    
    return features;
}

// Get the standard feature count (without game-specific values)
size_t AIInputFrame::GetStandardFeatureCount() const
{
    return 9; // player_index, frame_counter, health, opponent_health, pos_x, pos_y, opp_pos_x, opp_pos_y, distance
}

// Get the total feature count (standard + game-specific)
size_t AIInputFrame::GetTotalFeatureCount() const
{
    return GetStandardFeatureCount() + m_memory_values.size();
}

// Convert the frame to JSON format
std::string AIInputFrame::ToJson() const {
    nlohmann::json j = {
        {"player_index", m_player_index},
        {"frame_counter", m_frame_counter},
        {"health", m_health},
        {"opponent_health", m_opponent_health},
        {"position_x", m_position_x},
        {"position_y", m_position_y},
        {"opponent_position_x", m_opponent_position_x},
        {"opponent_position_y", m_opponent_position_y},
        {"distance_to_opponent", GetDistanceToOpponent()},
        {"memory_values", m_memory_values}
    };
    
    return j.dump();
}

// Load frame from JSON format
bool AIInputFrame::FromJson(const std::string& json_str) {
    try {
        nlohmann::json j = nlohmann::json::parse(json_str);
        
        // Load standard features
        m_player_index = j.value("player_index", 1);
        m_frame_counter = j.value("frame_counter", 0);
        m_health = j.value("health", 1.0f);
        m_opponent_health = j.value("opponent_health", 1.0f);
        m_position_x = j.value("position_x", 0.5f);
        m_position_y = j.value("position_y", 0.5f);
        m_opponent_position_x = j.value("opponent_position_x", 0.5f);
        m_opponent_position_y = j.value("opponent_position_y", 0.5f);
        
        // Load memory values
        if (j.contains("memory_values") && j["memory_values"].is_object()) {
            m_memory_values.clear();
            for (auto it = j["memory_values"].begin(); it != j["memory_values"].end(); ++it) {
                m_memory_values[it.key()] = it.value();
            }
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error parsing JSON: " << e.what() << std::endl;
        return false;
    }
}

// Add these member variables to the AIInputFrame class private section
std::vector<std::string> m_changedValues;
std::vector<std::string> m_significantChanges;

// Implement the new methods
bool AIInputFrame::extractFromMemoryWithChangeDetection(const AIMemoryMapping& mapping, 
                                                      bool onlySignificantChanges,
                                                      double changeThreshold) {
    // Clear previous change tracking
    m_changedValues.clear();
    m_significantChanges.clear();
    
    // Refresh memory values (this populates the internal changed mappings list)
    mapping.refreshValues(m_frameNumber);
    
    // Get the list of changed mappings
    m_changedValues = mapping.getChangedMappings();
    
    // Get significant changes
    m_significantChanges = mapping.getSignificantChanges(changeThreshold);
    
    // Determine which set of changes to use for extraction
    const std::vector<std::string>& changesToUse = 
        onlySignificantChanges ? m_significantChanges : m_changedValues;
    
    // If nothing changed significantly, just return false
    if (changesToUse.empty()) {
        return false;
    }
    
    // Update memory values in the frame
    for (const auto& name : changesToUse) {
        float value = 0.0f;
        if (mapping.readNormalizedValue(name, value)) {
            SetMemoryValue(name, value);
        }
    }
    
    // Always update standard features regardless of changes
    UpdateStandardFeaturesFromMapping(mapping);
    
    return !m_significantChanges.empty();
}

void AIInputFrame::UpdateStandardFeaturesFromMapping(const AIMemoryMapping& mapping) {
    // Try to update standard features from mapping
    float value = 0.0f;
    
    // Health values
    if (mapping.readNormalizedValue("p1_health", value)) {
        SetHealth(value);
    }
    
    if (mapping.readNormalizedValue("p2_health", value)) {
        SetOpponentHealth(value);
    }
    
    // Position values
    if (mapping.readNormalizedValue("p1_x_pos", value)) {
        SetPositionX(value);
    }
    
    if (mapping.readNormalizedValue("p1_y_pos", value)) {
        SetPositionY(value);
    }
    
    if (mapping.readNormalizedValue("p2_x_pos", value)) {
        SetOpponentPositionX(value);
    }
    
    if (mapping.readNormalizedValue("p2_y_pos", value)) {
        SetOpponentPositionY(value);
    }
    
    // Timer and round
    if (mapping.readNormalizedValue("timer", value)) {
        time_remaining = value;
    }
    
    if (mapping.readNormalizedValue("current_round", value) || 
        mapping.readNormalizedValue("round_number", value)) {
        round = value;
    }
    
    // Player state values
    if (mapping.readNormalizedValue("p1_state", value)) {
        p1_state = value;
    }
    
    if (mapping.readNormalizedValue("p2_state", value)) {
        p2_state = value;
    }
    
    // Calculate derived values
    CalculateDerivedValues();
}

void AIInputFrame::CalculateDerivedValues() {
    // Calculate distance between players
    float dx = GetOpponentPositionX() - GetPositionX();
    float dy = GetOpponentPositionY() - GetPositionY();
    
    // Normalize distance to 0-1 range (assuming positions are already normalized)
    x_distance = std::abs(dx);
    y_distance = std::abs(dy);
}

std::vector<std::string> AIInputFrame::getChangedValues() const {
    return m_changedValues;
}

std::vector<std::string> AIInputFrame::getSignificantChanges(double threshold) const {
    // If threshold matches what we already calculated, return cached result
    if (std::abs(threshold - 0.05) < 0.001) {
        return m_significantChanges;
    }
    
    // Otherwise, we'd need the mapping to recalculate, so just return empty
    return {};
}

bool AIInputFrame::captureCurrentState() {
    // This implementation would need to access emulator memory
    // to capture the current game state. Below is a stub implementation.
    
    // Check if a game is running
    if (!BurnDrvIsWorking) {
        return false;
    }
    
    // Update frame counter
    frameNumber++;
    
    // In a real implementation, you would:
    // 1. Read player positions from game memory
    // 2. Determine character states
    // 3. Calculate distances
    // 4. Read health, meter, and round status
    
    // For demonstration, we'll just simulate some values
    for (int i = 0; i < MAX_PLAYERS; i++) {
        playerValues[i].xPosition = 100.0f * (i + 1);
        playerValues[i].yPosition = 200.0f;
        playerValues[i].state = STATE_IDLE;
        playerValues[i].facingRight = (i == 0);
    }
    
    distanceBetweenPlayers = std::abs(playerValues[0].xPosition - playerValues[1].xPosition);
    horizontalDistance = distanceBetweenPlayers;
    verticalDistance = std::abs(playerValues[0].yPosition - playerValues[1].yPosition);
    
    p1Health = 100;
    p2Health = 100;
    p1Meter = 0;
    p2Meter = 0;
    
    return true;
}

} // namespace AI 