#pragma once

namespace AI {

/**
 * AIAction represents a discrete action that an AI can take.
 * This enum is used to map model outputs to game actions.
 */
enum AIAction {
    // No action
    NO_ACTION = 0,
    
    // Directional movements
    MOVE_UP = 1,
    MOVE_DOWN = 2,
    MOVE_LEFT = 3,
    MOVE_RIGHT = 4,
    MOVE_UP_LEFT = 5,
    MOVE_UP_RIGHT = 6,
    MOVE_DOWN_LEFT = 7,
    MOVE_DOWN_RIGHT = 8,
    
    // Basic attacks
    PUNCH = 9,
    KICK = 10,
    
    // Special moves
    SPECIAL_1 = 11,
    SPECIAL_2 = 12,
    SPECIAL_3 = 13,
    
    // Defensive moves
    BLOCK = 14,
    JUMP = 15,
    CROUCH = 16,
    
    // Combo actions
    COMBO_1 = 17,
    COMBO_2 = 18,
    COMBO_3 = 19,
    
    // System actions
    START = 20,
    COIN = 21,
    
    // Total number of actions
    ACTION_COUNT = 22
};

} // namespace AI 