#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize all AI components
 * @return 0 on success, non-zero on failure
 */
int AIModuleInit();

/**
 * @brief Shutdown all AI components
 * @return 0 on success
 */
int AIModuleExit();

/**
 * @brief Update AI systems each frame
 * Call this from the main emulation loop
 */
void AIModuleUpdate();

/**
 * @brief Handle game loading to load appropriate game-specific settings
 * @param game_name Name of the loaded game
 */
void AIModuleGameLoaded(const char* game_name);

#ifdef __cplusplus
}
#endif 