#ifndef GAME_DATABASE_H
#define GAME_DATABASE_H

#include "metal_declarations.h"

#ifdef __cplusplus
extern "C" {
#endif

// Game flags
#define GAME_FLAG_WORKING          (1 << 0)  // Game is fully working
#define GAME_FLAG_NOT_WORKING      (1 << 1)  // Game is not working
#define GAME_FLAG_IMPERFECT_SOUND  (1 << 2)  // Game has imperfect sound
#define GAME_FLAG_IMPERFECT_GFX    (1 << 3)  // Game has imperfect graphics
#define GAME_FLAG_MECHANICAL       (1 << 4)  // Game has mechanical parts
#define GAME_FLAG_REQUIRES_ARTWORK (1 << 5)  // Game requires external artwork
#define GAME_FLAG_SUPPORTS_SAVE    (1 << 6)  // Game supports save states
#define GAME_FLAG_PARENT           (1 << 7)  // Game is a parent
#define GAME_FLAG_CLONE            (1 << 8)  // Game is a clone
#define GAME_FLAG_FAVORITE         (1 << 9)  // Game is a favorite
#define GAME_FLAG_RECENTLY_PLAYED  (1 << 10) // Game was recently played

// Game types
typedef enum {
    GAME_TYPE_ARCADE,       // Arcade game
    GAME_TYPE_CONSOLE,      // Console game
    GAME_TYPE_COMPUTER,     // Computer game
    GAME_TYPE_PINBALL,      // Pinball game
    GAME_TYPE_QUIZ,         // Quiz game
    GAME_TYPE_MAZE,         // Maze game
    GAME_TYPE_SHOOTER,      // Shooter game
    GAME_TYPE_FIGHTING,     // Fighting game
    GAME_TYPE_BEAT_EM_UP,   // Beat'em up game
    GAME_TYPE_PLATFORM,     // Platform game
    GAME_TYPE_PUZZLE,       // Puzzle game
    GAME_TYPE_SPORTS,       // Sports game
    GAME_TYPE_RACING,       // Racing game
    GAME_TYPE_MISC,         // Miscellaneous game
    GAME_TYPE_COUNT         // Count of game types
} GameType;

// Game compatibility ratings
typedef enum {
    COMPATIBILITY_UNKNOWN,     // Compatibility not tested
    COMPATIBILITY_NONE,        // Game does not run
    COMPATIBILITY_POOR,        // Game runs poorly with major issues
    COMPATIBILITY_AVERAGE,     // Game runs with some issues
    COMPATIBILITY_GOOD,        // Game runs well with minor issues
    COMPATIBILITY_PERFECT      // Game runs perfectly
} GameCompatibility;

// Game database entry
typedef struct {
    const char* name;              // Short name (ROM name)
    const char* title;             // Full title
    const char* manufacturer;      // Manufacturer
    const char* year;              // Year of release
    const char* parent;            // Parent ROM name (if clone)
    unsigned int flags;            // Game flags
    GameType type;                 // Game type
    GameCompatibility compatibility; // Compatibility rating
    const char* comment;           // Comment/notes
    int nPlayers;                  // Number of players
    const char* path;              // Path to ROM (if found)
    const char* genre;             // Game genre
    int isFavorite;                // Is in favorites list
    time_t lastPlayed;             // Time last played
    int playCount;                 // Number of times played
    float rating;                  // User rating (0-5 stars)
} GameDatabaseEntry;

// Initialize game database
void GameDatabase_Init(void);

// Shutdown game database
void GameDatabase_Shutdown(void);

// Load game database from file
int GameDatabase_LoadFromFile(const char* filename);

// Save game database to file
int GameDatabase_SaveToFile(const char* filename);

// Get number of games in database
int GameDatabase_GetCount(void);

// Get game by index
const GameDatabaseEntry* GameDatabase_GetByIndex(int index);

// Get game by name
const GameDatabaseEntry* GameDatabase_GetByName(const char* name);

// Add game to database
int GameDatabase_AddGame(const GameDatabaseEntry* entry);

// Update game in database
int GameDatabase_UpdateGame(const char* name, const GameDatabaseEntry* entry);

// Remove game from database
int GameDatabase_RemoveGame(const char* name);

// Get filtered game list
int GameDatabase_GetFilteredList(GameDatabaseEntry** outList, int maxEntries, 
                               unsigned int includeFlags, unsigned int excludeFlags, 
                               GameType type, GameCompatibility minCompatibility);

// Search games by title or name
int GameDatabase_SearchGames(GameDatabaseEntry** outList, int maxEntries, const char* searchTerm);

// Toggle favorite status
int GameDatabase_ToggleFavorite(const char* name, int isFavorite);

// Update last played time
void GameDatabase_UpdateLastPlayed(const char* name);

// Get recently played games
int GameDatabase_GetRecentlyPlayed(GameDatabaseEntry** outList, int maxEntries);

// Scan directory for ROMs
int GameDatabase_ScanDirectory(const char* directory);

// Update compatibility rating
void GameDatabase_UpdateCompatibility(const char* name, GameCompatibility rating);

// Set user rating
void GameDatabase_SetRating(const char* name, float rating);

// Get favorites
int GameDatabase_GetFavorites(GameDatabaseEntry** outList, int maxEntries);

#ifdef __cplusplus
}
#endif

#endif // GAME_DATABASE_H 