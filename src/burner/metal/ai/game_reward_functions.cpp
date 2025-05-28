#include "ai_rl_integration.h"
#include "ai_input_frame.h"
#include "ai_output_action.h"
#include <cmath>
#include <algorithm>

namespace fbneo {
namespace ai {

// Helper functions for reward calculation
namespace {
    // Calculate distance between two points
    float calculateDistance(float x1, float y1, float x2, float y2) {
        return std::sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
    }
    
    // Calculate health change between frames
    float calculateHealthChange(int prevHealth, int currHealth) {
        return static_cast<float>(currHealth - prevHealth);
    }
    
    // Calculate score change between frames
    float calculateScoreChange(int prevScore, int currScore) {
        return static_cast<float>(currScore - prevScore);
    }
    
    // Normalize value to range [0, 1]
    float normalize(float value, float min, float max) {
        if (max == min) return 0.5f;
        return std::clamp((value - min) / (max - min), 0.0f, 1.0f);
    }
    
    // Extract player positions from game state
    void extractPositions(const AIInputFrame& state, float& playerX, float& playerY, 
                         float& opponentX, float& opponentY) {
        // Default values
        playerX = playerY = opponentX = opponentY = 0.0f;
        
        if (state.gameState.data) {
            // Cast to appropriate struct based on game type
            if (state.gameState.gameType == GAME_FIGHTING) {
                const auto* fightingState = reinterpret_cast<const FightingGameState*>(state.gameState.data);
                playerX = static_cast<float>(fightingState->playerX);
                playerY = static_cast<float>(fightingState->playerY);
                opponentX = static_cast<float>(fightingState->opponentX);
                opponentY = static_cast<float>(fightingState->opponentY);
            }
            else if (state.gameState.gameType == GAME_SHOOTER) {
                const auto* shooterState = reinterpret_cast<const ShooterGameState*>(state.gameState.data);
                playerX = static_cast<float>(shooterState->playerX);
                playerY = static_cast<float>(shooterState->playerY);
                // Use first enemy position as opponent (if available)
                if (shooterState->enemyCount > 0) {
                    opponentX = static_cast<float>(shooterState->enemies[0].x);
                    opponentY = static_cast<float>(shooterState->enemies[0].y);
                }
            }
            else if (state.gameState.gameType == GAME_PLATFORMER) {
                const auto* platformerState = reinterpret_cast<const PlatformerGameState*>(state.gameState.data);
                playerX = static_cast<float>(platformerState->playerX);
                playerY = static_cast<float>(platformerState->playerY);
                // Use first enemy position as opponent (if available)
                if (platformerState->enemyCount > 0) {
                    opponentX = static_cast<float>(platformerState->enemies[0].x);
                    opponentY = static_cast<float>(platformerState->enemies[0].y);
                }
            }
        }
    }
}

// Default reward function (used if no game-specific function is set)
float RLIntegration::defaultReward(const AIInputFrame& prevState, 
                                  const AIInputFrame& currState, 
                                  const AIOutputAction& action) {
    // Very simple reward: +1 for survival, -1 for game over
    return currState.gameState.gameOver ? -1.0f : 0.01f;
}

// Fighting game reward function
RewardFunction createFightingGameReward() {
    return [](const AIInputFrame& prevState, const AIInputFrame& currState, const AIOutputAction& action) {
        float reward = 0.0f;
        
        // Extract game states
        const auto* prevFightingState = reinterpret_cast<const FightingGameState*>(
            prevState.gameState.data && prevState.gameState.gameType == GAME_FIGHTING ? 
            prevState.gameState.data : nullptr);
            
        const auto* currFightingState = reinterpret_cast<const FightingGameState*>(
            currState.gameState.data && currState.gameState.gameType == GAME_FIGHTING ? 
            currState.gameState.data : nullptr);
        
        if (!prevFightingState || !currFightingState) {
            return 0.0f; // Can't calculate reward without valid states
        }
        
        // Reward for damaging opponent
        int opponentHealthDelta = prevFightingState->opponentHealth - currFightingState->opponentHealth;
        if (opponentHealthDelta > 0) {
            reward += opponentHealthDelta * 0.1f;
            
            // Bonus for big hits
            if (opponentHealthDelta > 10) {
                reward += 0.5f;
            }
        }
        
        // Penalty for taking damage
        int playerHealthDelta = prevFightingState->playerHealth - currFightingState->playerHealth;
        if (playerHealthDelta > 0) {
            reward -= playerHealthDelta * 0.15f;
        }
        
        // Reward for blocking attacks (health didn't decrease but was hit)
        if (playerHealthDelta == 0 && currFightingState->isBlocking && currFightingState->wasHit) {
            reward += 0.3f;
        }
        
        // Reward for executing special moves
        if (currFightingState->specialMoveExecuted) {
            reward += 0.5f;
        }
        
        // Reward for combos (multiple hits)
        if (currFightingState->comboCounter > prevFightingState->comboCounter) {
            reward += 0.2f * (currFightingState->comboCounter - prevFightingState->comboCounter);
        }
        
        // Major rewards/penalties for round events
        if (currFightingState->roundWon && !prevFightingState->roundWon) {
            reward += 5.0f; // Big reward for winning a round
        }
        
        if (currFightingState->roundLost && !prevFightingState->roundLost) {
            reward -= 5.0f; // Big penalty for losing a round
        }
        
        // Major rewards for match events
        if (currFightingState->matchWon && !prevFightingState->matchWon) {
            reward += 10.0f; // Very big reward for winning the match
        }
        
        // Small penalty for button mashing (pressing too many buttons at once)
        int buttonCount = 0;
        for (int i = 0; i < MAX_BUTTONS; i++) {
            if (action.buttons[i]) buttonCount++;
        }
        
        if (buttonCount > 2) {
            reward -= 0.01f * (buttonCount - 2);
        }
        
        // Small time penalty to encourage efficient play
        reward -= 0.001f;
        
        return reward;
    };
}

// Platformer game reward function
RewardFunction createPlatformerReward() {
    return [](const AIInputFrame& prevState, const AIInputFrame& currState, const AIOutputAction& action) {
        float reward = 0.0f;
        
        // Extract game states
        const auto* prevPlatformerState = reinterpret_cast<const PlatformerGameState*>(
            prevState.gameState.data && prevState.gameState.gameType == GAME_PLATFORMER ? 
            prevState.gameState.data : nullptr);
            
        const auto* currPlatformerState = reinterpret_cast<const PlatformerGameState*>(
            currState.gameState.data && currState.gameState.gameType == GAME_PLATFORMER ? 
            currState.gameState.data : nullptr);
        
        if (!prevPlatformerState || !currPlatformerState) {
            return 0.0f; // Can't calculate reward without valid states
        }
        
        // Reward for collecting coins/items
        if (currPlatformerState->score > prevPlatformerState->score) {
            float scoreDelta = static_cast<float>(currPlatformerState->score - prevPlatformerState->score);
            reward += 0.01f * scoreDelta;
        }
        
        // Reward for progressing horizontally (mostly right in classic platformers)
        if (currPlatformerState->playerX > prevPlatformerState->playerX) {
            float progressDelta = currPlatformerState->playerX - prevPlatformerState->playerX;
            reward += 0.005f * progressDelta;
        }
        
        // Penalty for going backwards (usually not desirable)
        if (currPlatformerState->playerX < prevPlatformerState->playerX) {
            float backtrackDelta = prevPlatformerState->playerX - currPlatformerState->playerX;
            // Smaller penalty as sometimes backtracking is needed
            reward -= 0.001f * backtrackDelta;
        }
        
        // Reward for surviving (staying alive)
        if (currPlatformerState->lives == prevPlatformerState->lives) {
            reward += 0.001f;
        }
        
        // Penalty for losing a life
        if (currPlatformerState->lives < prevPlatformerState->lives) {
            reward -= 1.0f;
        }
        
        // Reward for defeating enemies
        if (currPlatformerState->enemiesDefeated > prevPlatformerState->enemiesDefeated) {
            int enemiesDelta = currPlatformerState->enemiesDefeated - prevPlatformerState->enemiesDefeated;
            reward += 0.2f * enemiesDelta;
        }
        
        // Reward for completing level
        if (currPlatformerState->level > prevPlatformerState->level) {
            reward += 5.0f;
        }
        
        // Reward for getting power-ups
        if (currPlatformerState->powerUpState > prevPlatformerState->powerUpState) {
            reward += 0.5f;
        }
        
        // Penalty for losing power-ups
        if (currPlatformerState->powerUpState < prevPlatformerState->powerUpState) {
            reward -= 0.3f;
        }
        
        // Penalty for being in a "hurt" state
        if (currPlatformerState->isHurt) {
            reward -= 0.1f;
        }
        
        // Small time penalty to encourage efficient play
        reward -= 0.0005f;
        
        return reward;
    };
}

// Puzzle game reward function
RewardFunction createPuzzleGameReward() {
    return [](const AIInputFrame& prevState, const AIInputFrame& currState, const AIOutputAction& action) {
        float reward = 0.0f;
        
        // Extract game states
        const auto* prevPuzzleState = reinterpret_cast<const PuzzleGameState*>(
            prevState.gameState.data && prevState.gameState.gameType == GAME_PUZZLE ? 
            prevState.gameState.data : nullptr);
            
        const auto* currPuzzleState = reinterpret_cast<const PuzzleGameState*>(
            currState.gameState.data && currState.gameState.gameType == GAME_PUZZLE ? 
            currState.gameState.data : nullptr);
        
        if (!prevPuzzleState || !currPuzzleState) {
            return 0.0f; // Can't calculate reward without valid states
        }
        
        // Reward for clearing lines (Tetris-style)
        if (currPuzzleState->linesCleared > prevPuzzleState->linesCleared) {
            int linesDelta = currPuzzleState->linesCleared - prevPuzzleState->linesCleared;
            
            // Give exponentially higher rewards for more lines cleared at once
            switch (linesDelta) {
                case 1: reward += 0.2f; break;
                case 2: reward += 0.5f; break;
                case 3: reward += 1.2f; break;
                case 4: reward += 3.0f; break;  // Tetris!
                default: reward += 0.2f * linesDelta; break;
            }
        }
        
        // Reward for score increases
        if (currPuzzleState->score > prevPuzzleState->score) {
            float scoreDelta = static_cast<float>(currPuzzleState->score - prevPuzzleState->score);
            reward += 0.001f * scoreDelta;
        }
        
        // Penalty for game over
        if (currPuzzleState->gameOver && !prevPuzzleState->gameOver) {
            reward -= 5.0f;
        }
        
        // Reward for leveling up
        if (currPuzzleState->level > prevPuzzleState->level) {
            reward += 1.0f;
        }
        
        // Reward for making combos (puzzle chain reactions)
        if (currPuzzleState->comboCounter > prevPuzzleState->comboCounter) {
            reward += 0.3f * (currPuzzleState->comboCounter - prevPuzzleState->comboCounter);
        }
        
        // Penalty for increasing tower height (in block-stacking games)
        // Higher tower means closer to losing
        if (currPuzzleState->stackHeight > prevPuzzleState->stackHeight) {
            float heightDelta = static_cast<float>(currPuzzleState->stackHeight - prevPuzzleState->stackHeight);
            reward -= 0.05f * heightDelta;
        }
        
        // Reward for decreasing tower height (cleared some blocks)
        if (currPuzzleState->stackHeight < prevPuzzleState->stackHeight) {
            float heightDelta = static_cast<float>(prevPuzzleState->stackHeight - currPuzzleState->stackHeight);
            reward += 0.1f * heightDelta;
        }
        
        // Small time penalty to encourage efficient play
        reward -= 0.0002f;
        
        return reward;
    };
}

// Shooter game reward function
RewardFunction createShooterReward() {
    return [](const AIInputFrame& prevState, const AIInputFrame& currState, const AIOutputAction& action) {
        float reward = 0.0f;
        
        // Extract game states
        const auto* prevShooterState = reinterpret_cast<const ShooterGameState*>(
            prevState.gameState.data && prevState.gameState.gameType == GAME_SHOOTER ? 
            prevState.gameState.data : nullptr);
            
        const auto* currShooterState = reinterpret_cast<const ShooterGameState*>(
            currState.gameState.data && currState.gameState.gameType == GAME_SHOOTER ? 
            currState.gameState.data : nullptr);
        
        if (!prevShooterState || !currShooterState) {
            return 0.0f; // Can't calculate reward without valid states
        }
        
        // Reward for destroying enemies
        if (currShooterState->enemiesDestroyed > prevShooterState->enemiesDestroyed) {
            int enemiesDelta = currShooterState->enemiesDestroyed - prevShooterState->enemiesDestroyed;
            reward += 0.2f * enemiesDelta;
        }
        
        // Reward for score increases
        if (currShooterState->score > prevShooterState->score) {
            float scoreDelta = static_cast<float>(currShooterState->score - prevShooterState->score);
            reward += 0.001f * scoreDelta;
        }
        
        // Penalty for taking damage
        if (currShooterState->playerHealth < prevShooterState->playerHealth) {
            float healthDelta = static_cast<float>(prevShooterState->playerHealth - currShooterState->playerHealth);
            reward -= 0.5f * healthDelta;
        }
        
        // Penalty for losing a life
        if (currShooterState->lives < prevShooterState->lives) {
            reward -= 2.0f;
        }
        
        // Penalty for game over
        if (currShooterState->gameOver && !prevShooterState->gameOver) {
            reward -= 5.0f;
        }
        
        // Reward for collecting power-ups
        if (currShooterState->powerUpLevel > prevShooterState->powerUpLevel) {
            reward += 0.5f;
        }
        
        // Reward for progressing to next level/stage
        if (currShooterState->level > prevShooterState->level) {
            reward += 3.0f;
        }
        
        // Reward for boss damage (if applicable)
        if (currShooterState->bossActive && 
            currShooterState->bossHealth < prevShooterState->bossHealth) {
            float bossDamage = static_cast<float>(prevShooterState->bossHealth - currShooterState->bossHealth);
            reward += 0.1f * bossDamage;
        }
        
        // Big reward for defeating a boss
        if (prevShooterState->bossActive && !currShooterState->bossActive &&
            currShooterState->bossHealth <= 0) {
            reward += 5.0f;
        }
        
        // Reward for firing (encourages attacking)
        if (action.buttons[0]) { // Assuming button 0 is fire
            reward += 0.001f;
        }
        
        // Reward for movement (encourages dodging)
        if (action.left || action.right || action.up || action.down) {
            reward += 0.0005f;
        }
        
        // Small time penalty to encourage efficient play
        reward -= 0.0001f;
        
        return reward;
    };
}

// Racing game reward function
RewardFunction createRacingGameReward() {
    return [](const AIInputFrame& prevState, const AIInputFrame& currState, const AIOutputAction& action) {
        float reward = 0.0f;
        
        // Extract game states
        const auto* prevRacingState = reinterpret_cast<const RacingGameState*>(
            prevState.gameState.data && prevState.gameState.gameType == GAME_RACING ? 
            prevState.gameState.data : nullptr);
            
        const auto* currRacingState = reinterpret_cast<const RacingGameState*>(
            currState.gameState.data && currState.gameState.gameType == GAME_RACING ? 
            currState.gameState.data : nullptr);
        
        if (!prevRacingState || !currRacingState) {
            return 0.0f; // Can't calculate reward without valid states
        }
        
        // Reward for speed
        reward += 0.001f * currRacingState->speed;
        
        // Reward for progress around the track
        if (currRacingState->trackProgress > prevRacingState->trackProgress) {
            float progressDelta = currRacingState->trackProgress - prevRacingState->trackProgress;
            reward += 0.1f * progressDelta;
        }
        
        // Penalty for going backwards
        if (currRacingState->trackProgress < prevRacingState->trackProgress) {
            float regressionDelta = prevRacingState->trackProgress - currRacingState->trackProgress;
            reward -= 0.2f * regressionDelta;
        }
        
        // Penalty for collisions
        if (currRacingState->collisionFlag && !prevRacingState->collisionFlag) {
            reward -= 0.5f;
        }
        
        // Penalty for going off-track
        if (currRacingState->offTrack && !prevRacingState->offTrack) {
            reward -= 0.3f;
        }
        
        // Reward for being on-track
        if (!currRacingState->offTrack && prevRacingState->offTrack) {
            reward += 0.2f;
        }
        
        // Reward for overtaking
        if (currRacingState->position < prevRacingState->position) {
            int positionDelta = prevRacingState->position - currRacingState->position;
            reward += 0.5f * positionDelta;
        }
        
        // Penalty for being overtaken
        if (currRacingState->position > prevRacingState->position) {
            int positionDelta = currRacingState->position - prevRacingState->position;
            reward -= 0.3f * positionDelta;
        }
        
        // Reward for completing a lap
        if (currRacingState->lap > prevRacingState->lap) {
            reward += 2.0f;
        }
        
        // Reward for finishing the race
        if (currRacingState->raceComplete && !prevRacingState->raceComplete) {
            // Reward based on position - higher for better positions
            reward += 10.0f / currRacingState->position;
        }
        
        // Small time penalty to encourage finishing quickly
        reward -= 0.0005f;
        
        return reward;
    };
}

// Factory function to create the appropriate reward function based on game type
RewardFunction RLIntegration::createRewardFunction(const std::string& gameType) {
    if (gameType == "fighting") {
        return createFightingGameReward();
    }
    else if (gameType == "platformer") {
        return createPlatformerReward();
    }
    else if (gameType == "puzzle") {
        return createPuzzleGameReward();
    }
    else if (gameType == "shooter") {
        return createShooterReward();
    }
    else if (gameType == "racing") {
        return createRacingGameReward();
    }
    else {
        // Unknown game type, return default reward function
        return std::bind(&RLIntegration::defaultReward, this, 
                        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    }
}

} // namespace ai
} // namespace fbneo 