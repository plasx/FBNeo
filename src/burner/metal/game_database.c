#include "game_database.h"
#include "rom_loading_debug.h"
#include "memory_tracking.h"
#include "error_handling.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ctype.h>

// Maximum number of games in database
#define MAX_GAMES 20000

// Default database filename
#define DEFAULT_DATABASE_FILE "fbneo_games.db"

// Game database storage
static GameDatabaseEntry* g_games = NULL;
static int g_gameCount = 0;
static int g_initialized = 0;

// Copy string with allocation
static char* StrDup(const char* str) {
    if (!str) return NULL;
    
    char* result = (char*)malloc(strlen(str) + 1);
    if (result) {
        strcpy(result, str);
    }
    return result;
}

// Case-insensitive string comparison
static int StrCaseCompare(const char* str1, const char* str2) {
    if (!str1 && !str2) return 0;
    if (!str1) return -1;
    if (!str2) return 1;
    
    while (*str1 && *str2) {
        int c1 = tolower(*str1);
        int c2 = tolower(*str2);
        if (c1 != c2) {
            return c1 - c2;
        }
        str1++;
        str2++;
    }
    
    if (*str1) return 1;
    if (*str2) return -1;
    return 0;
}

// Initialize game database
void GameDatabase_Init(void) {
    if (g_initialized) {
        return;
    }
    
    // Allocate memory for games
    g_games = (GameDatabaseEntry*)MemoryTracker_Allocate(MAX_GAMES * sizeof(GameDatabaseEntry), "Game Database");
    if (!g_games) {
        ErrorHandler_ReportError(ERROR_OUT_OF_MEMORY, ERROR_SEVERITY_ERROR, ERROR_CATEGORY_SYSTEM,
                               "Failed to allocate memory for game database", 
                               "Could not allocate memory for MAX_GAMES game entries",
                               "GameDatabase_Init");
        return;
    }
    
    // Initialize game count
    g_gameCount = 0;
    
    // Flag as initialized
    g_initialized = 1;
    
    ROMLoader_TrackLoadStep("DATABASE INIT", "Game database system initialized (max %d games)", MAX_GAMES);
    
    // Try to load default database
    if (GameDatabase_LoadFromFile(DEFAULT_DATABASE_FILE) != 0) {
        // If not found, add some predefined entries for testing
        ROMLoader_DebugLog(LOG_INFO, "Default database not found, adding test entries");
        
        // Add a few test entries for popular games
        GameDatabaseEntry testGames[] = {
            {
                "mvsc", "Marvel vs. Capcom: Clash of Super Heroes", "Capcom", "1998",
                NULL, GAME_FLAG_WORKING | GAME_FLAG_SUPPORTS_SAVE, 
                GAME_TYPE_FIGHTING, COMPATIBILITY_PERFECT,
                "CPS2 fighting game", 2, NULL, "Fighting", 0, 0, 0, 5.0f
            },
            {
                "sfa3", "Street Fighter Alpha 3", "Capcom", "1998",
                NULL, GAME_FLAG_WORKING | GAME_FLAG_SUPPORTS_SAVE, 
                GAME_TYPE_FIGHTING, COMPATIBILITY_PERFECT,
                "CPS2 fighting game", 2, NULL, "Fighting", 0, 0, 0, 4.5f
            },
            {
                "mslug", "Metal Slug - Super Vehicle-001", "Nazca", "1996",
                NULL, GAME_FLAG_WORKING | GAME_FLAG_SUPPORTS_SAVE, 
                GAME_TYPE_PLATFORM, COMPATIBILITY_PERFECT,
                "Neo Geo run'n'gun", 2, NULL, "Run'n'gun", 0, 0, 0, 4.8f
            },
            {
                "dino", "Cadillacs and Dinosaurs", "Capcom", "1993",
                NULL, GAME_FLAG_WORKING | GAME_FLAG_SUPPORTS_SAVE, 
                GAME_TYPE_BEAT_EM_UP, COMPATIBILITY_PERFECT,
                "CPS1 beat'em up", 3, NULL, "Beat'em up", 0, 0, 0, 4.7f
            },
            {
                "kof98", "The King of Fighters '98", "SNK", "1998",
                NULL, GAME_FLAG_WORKING | GAME_FLAG_SUPPORTS_SAVE, 
                GAME_TYPE_FIGHTING, COMPATIBILITY_PERFECT,
                "Neo Geo fighting game", 2, NULL, "Fighting", 0, 0, 0, 4.9f
            }
        };
        
        // Add test entries
        for (int i = 0; i < sizeof(testGames) / sizeof(testGames[0]); i++) {
            GameDatabase_AddGame(&testGames[i]);
        }
    }
}

// Shutdown game database
void GameDatabase_Shutdown(void) {
    if (!g_initialized) {
        return;
    }
    
    // Save database if there are games
    if (g_gameCount > 0) {
        GameDatabase_SaveToFile(DEFAULT_DATABASE_FILE);
    }
    
    // Free strings in each entry
    for (int i = 0; i < g_gameCount; i++) {
        free((void*)g_games[i].name);
        free((void*)g_games[i].title);
        free((void*)g_games[i].manufacturer);
        free((void*)g_games[i].year);
        free((void*)g_games[i].parent);
        free((void*)g_games[i].comment);
        free((void*)g_games[i].path);
        free((void*)g_games[i].genre);
    }
    
    // Free the games array
    MemoryTracker_Free(g_games);
    g_games = NULL;
    g_gameCount = 0;
    g_initialized = 0;
    
    ROMLoader_DebugLog(LOG_INFO, "Game database shutdown");
}

// Load game database from file
int GameDatabase_LoadFromFile(const char* filename) {
    if (!g_initialized || !filename) {
        return -1;
    }
    
    FILE* file = fopen(filename, "rb");
    if (!file) {
        ROMLoader_DebugLog(LOG_WARNING, "Failed to open game database file: %s", filename);
        return -1;
    }
    
    // Clear existing database
    for (int i = 0; i < g_gameCount; i++) {
        free((void*)g_games[i].name);
        free((void*)g_games[i].title);
        free((void*)g_games[i].manufacturer);
        free((void*)g_games[i].year);
        free((void*)g_games[i].parent);
        free((void*)g_games[i].comment);
        free((void*)g_games[i].path);
        free((void*)g_games[i].genre);
    }
    g_gameCount = 0;
    
    // Read header
    char header[16];
    if (fread(header, 1, 16, file) != 16 || memcmp(header, "FBNEO_GAMEDB_V1", 15) != 0) {
        ROMLoader_DebugLog(LOG_ERROR, "Invalid game database file format");
        fclose(file);
        return -1;
    }
    
    // Read game count
    int count;
    if (fread(&count, sizeof(int), 1, file) != 1) {
        ROMLoader_DebugLog(LOG_ERROR, "Failed to read game count from database");
        fclose(file);
        return -1;
    }
    
    // Sanity check
    if (count <= 0 || count > MAX_GAMES) {
        ROMLoader_DebugLog(LOG_ERROR, "Invalid game count in database: %d", count);
        fclose(file);
        return -1;
    }
    
    ROMLoader_DebugLog(LOG_INFO, "Loading %d games from database", count);
    
    // Read each game entry
    for (int i = 0; i < count; i++) {
        GameDatabaseEntry entry;
        memset(&entry, 0, sizeof(entry));
        
        // Read fixed-size fields
        fread(&entry.flags, sizeof(entry.flags), 1, file);
        fread(&entry.type, sizeof(entry.type), 1, file);
        fread(&entry.compatibility, sizeof(entry.compatibility), 1, file);
        fread(&entry.nPlayers, sizeof(entry.nPlayers), 1, file);
        fread(&entry.isFavorite, sizeof(entry.isFavorite), 1, file);
        fread(&entry.lastPlayed, sizeof(entry.lastPlayed), 1, file);
        fread(&entry.playCount, sizeof(entry.playCount), 1, file);
        fread(&entry.rating, sizeof(entry.rating), 1, file);
        
        // Read strings with length prefixes
        int len;
        
        #define READ_STRING(field) \
            fread(&len, sizeof(int), 1, file); \
            if (len > 0) { \
                char* str = (char*)malloc(len + 1); \
                fread(str, 1, len, file); \
                str[len] = '\0'; \
                entry.field = str; \
            } else { \
                entry.field = NULL; \
            }
        
        READ_STRING(name);
        READ_STRING(title);
        READ_STRING(manufacturer);
        READ_STRING(year);
        READ_STRING(parent);
        READ_STRING(comment);
        READ_STRING(path);
        READ_STRING(genre);
        
        #undef READ_STRING
        
        // Add to database
        g_games[g_gameCount++] = entry;
    }
    
    fclose(file);
    
    ROMLoader_TrackLoadStep("DATABASE INIT", "Loaded %d games from database: %s",
                         g_gameCount, filename);
    
    return 0;
}

// Save game database to file
int GameDatabase_SaveToFile(const char* filename) {
    if (!g_initialized || !filename || g_gameCount <= 0) {
        return -1;
    }
    
    FILE* file = fopen(filename, "wb");
    if (!file) {
        ROMLoader_DebugLog(LOG_ERROR, "Failed to create game database file: %s", filename);
        return -1;
    }
    
    // Write header
    fwrite("FBNEO_GAMEDB_V1\0", 1, 16, file);
    
    // Write game count
    fwrite(&g_gameCount, sizeof(int), 1, file);
    
    // Write each game entry
    for (int i = 0; i < g_gameCount; i++) {
        GameDatabaseEntry* entry = &g_games[i];
        
        // Write fixed-size fields
        fwrite(&entry->flags, sizeof(entry->flags), 1, file);
        fwrite(&entry->type, sizeof(entry->type), 1, file);
        fwrite(&entry->compatibility, sizeof(entry->compatibility), 1, file);
        fwrite(&entry->nPlayers, sizeof(entry->nPlayers), 1, file);
        fwrite(&entry->isFavorite, sizeof(entry->isFavorite), 1, file);
        fwrite(&entry->lastPlayed, sizeof(entry->lastPlayed), 1, file);
        fwrite(&entry->playCount, sizeof(entry->playCount), 1, file);
        fwrite(&entry->rating, sizeof(entry->rating), 1, file);
        
        // Write strings with length prefixes
        #define WRITE_STRING(field) \
            if (entry->field) { \
                int len = strlen(entry->field); \
                fwrite(&len, sizeof(int), 1, file); \
                fwrite(entry->field, 1, len, file); \
            } else { \
                int len = 0; \
                fwrite(&len, sizeof(int), 1, file); \
            }
        
        WRITE_STRING(name);
        WRITE_STRING(title);
        WRITE_STRING(manufacturer);
        WRITE_STRING(year);
        WRITE_STRING(parent);
        WRITE_STRING(comment);
        WRITE_STRING(path);
        WRITE_STRING(genre);
        
        #undef WRITE_STRING
    }
    
    fclose(file);
    
    ROMLoader_DebugLog(LOG_INFO, "Saved %d games to database: %s", g_gameCount, filename);
    
    return 0;
}

// Get number of games in database
int GameDatabase_GetCount(void) {
    return g_gameCount;
}

// Get game by index
const GameDatabaseEntry* GameDatabase_GetByIndex(int index) {
    if (!g_initialized || index < 0 || index >= g_gameCount) {
        return NULL;
    }
    
    return &g_games[index];
}

// Get game by name
const GameDatabaseEntry* GameDatabase_GetByName(const char* name) {
    if (!g_initialized || !name) {
        return NULL;
    }
    
    for (int i = 0; i < g_gameCount; i++) {
        if (g_games[i].name && StrCaseCompare(g_games[i].name, name) == 0) {
            return &g_games[i];
        }
    }
    
    return NULL;
}

// Add game to database
int GameDatabase_AddGame(const GameDatabaseEntry* entry) {
    if (!g_initialized || !entry || !entry->name) {
        return -1;
    }
    
    // Check if game already exists
    for (int i = 0; i < g_gameCount; i++) {
        if (g_games[i].name && StrCaseCompare(g_games[i].name, entry->name) == 0) {
            ROMLoader_DebugLog(LOG_WARNING, "Game already exists in database: %s", entry->name);
            return -1;
        }
    }
    
    // Check if database is full
    if (g_gameCount >= MAX_GAMES) {
        ROMLoader_DebugLog(LOG_ERROR, "Game database is full, can't add: %s", entry->name);
        return -1;
    }
    
    // Copy entry to database
    GameDatabaseEntry* newEntry = &g_games[g_gameCount];
    
    // Copy strings
    newEntry->name = StrDup(entry->name);
    newEntry->title = StrDup(entry->title);
    newEntry->manufacturer = StrDup(entry->manufacturer);
    newEntry->year = StrDup(entry->year);
    newEntry->parent = StrDup(entry->parent);
    newEntry->comment = StrDup(entry->comment);
    newEntry->path = StrDup(entry->path);
    newEntry->genre = StrDup(entry->genre);
    
    // Copy other fields
    newEntry->flags = entry->flags;
    newEntry->type = entry->type;
    newEntry->compatibility = entry->compatibility;
    newEntry->nPlayers = entry->nPlayers;
    newEntry->isFavorite = entry->isFavorite;
    newEntry->lastPlayed = entry->lastPlayed;
    newEntry->playCount = entry->playCount;
    newEntry->rating = entry->rating;
    
    // Increment game count
    g_gameCount++;
    
    ROMLoader_DebugLog(LOG_INFO, "Added game to database: %s (%s)", entry->name, entry->title);
    
    return 0;
}

// Update game in database
int GameDatabase_UpdateGame(const char* name, const GameDatabaseEntry* entry) {
    if (!g_initialized || !name || !entry) {
        return -1;
    }
    
    // Find the game
    int index = -1;
    for (int i = 0; i < g_gameCount; i++) {
        if (g_games[i].name && StrCaseCompare(g_games[i].name, name) == 0) {
            index = i;
            break;
        }
    }
    
    if (index < 0) {
        ROMLoader_DebugLog(LOG_WARNING, "Game not found in database: %s", name);
        return -1;
    }
    
    // Free existing strings
    free((void*)g_games[index].name);
    free((void*)g_games[index].title);
    free((void*)g_games[index].manufacturer);
    free((void*)g_games[index].year);
    free((void*)g_games[index].parent);
    free((void*)g_games[index].comment);
    free((void*)g_games[index].path);
    free((void*)g_games[index].genre);
    
    // Copy strings from new entry
    g_games[index].name = StrDup(entry->name);
    g_games[index].title = StrDup(entry->title);
    g_games[index].manufacturer = StrDup(entry->manufacturer);
    g_games[index].year = StrDup(entry->year);
    g_games[index].parent = StrDup(entry->parent);
    g_games[index].comment = StrDup(entry->comment);
    g_games[index].path = StrDup(entry->path);
    g_games[index].genre = StrDup(entry->genre);
    
    // Copy other fields
    g_games[index].flags = entry->flags;
    g_games[index].type = entry->type;
    g_games[index].compatibility = entry->compatibility;
    g_games[index].nPlayers = entry->nPlayers;
    g_games[index].isFavorite = entry->isFavorite;
    g_games[index].lastPlayed = entry->lastPlayed;
    g_games[index].playCount = entry->playCount;
    g_games[index].rating = entry->rating;
    
    ROMLoader_DebugLog(LOG_INFO, "Updated game in database: %s", name);
    
    return 0;
}

// Remove game from database
int GameDatabase_RemoveGame(const char* name) {
    if (!g_initialized || !name) {
        return -1;
    }
    
    // Find the game
    int index = -1;
    for (int i = 0; i < g_gameCount; i++) {
        if (g_games[i].name && StrCaseCompare(g_games[i].name, name) == 0) {
            index = i;
            break;
        }
    }
    
    if (index < 0) {
        ROMLoader_DebugLog(LOG_WARNING, "Game not found in database: %s", name);
        return -1;
    }
    
    // Free strings
    free((void*)g_games[index].name);
    free((void*)g_games[index].title);
    free((void*)g_games[index].manufacturer);
    free((void*)g_games[index].year);
    free((void*)g_games[index].parent);
    free((void*)g_games[index].comment);
    free((void*)g_games[index].path);
    free((void*)g_games[index].genre);
    
    // Shift remaining games
    if (index < g_gameCount - 1) {
        memmove(&g_games[index], &g_games[index + 1], 
                (g_gameCount - index - 1) * sizeof(GameDatabaseEntry));
    }
    
    // Decrement game count
    g_gameCount--;
    
    ROMLoader_DebugLog(LOG_INFO, "Removed game from database: %s", name);
    
    return 0;
}

// Get filtered game list
int GameDatabase_GetFilteredList(GameDatabaseEntry** outList, int maxEntries, 
                               unsigned int includeFlags, unsigned int excludeFlags, 
                               GameType type, GameCompatibility minCompatibility) {
    if (!g_initialized || !outList || maxEntries <= 0) {
        return -1;
    }
    
    int count = 0;
    
    for (int i = 0; i < g_gameCount && count < maxEntries; i++) {
        // Check flags
        if (includeFlags && !(g_games[i].flags & includeFlags)) {
            continue;
        }
        
        if (excludeFlags && (g_games[i].flags & excludeFlags)) {
            continue;
        }
        
        // Check game type
        if (type != GAME_TYPE_COUNT && g_games[i].type != type) {
            continue;
        }
        
        // Check compatibility
        if (minCompatibility != COMPATIBILITY_UNKNOWN && 
            g_games[i].compatibility < minCompatibility) {
            continue;
        }
        
        // Add to output list
        outList[count++] = &g_games[i];
    }
    
    return count;
}

// Search games by title or name
int GameDatabase_SearchGames(GameDatabaseEntry** outList, int maxEntries, const char* searchTerm) {
    if (!g_initialized || !outList || maxEntries <= 0 || !searchTerm) {
        return -1;
    }
    
    int count = 0;
    char searchLower[256];
    
    // Convert search term to lowercase
    size_t searchLen = strlen(searchTerm);
    if (searchLen >= sizeof(searchLower)) {
        searchLen = sizeof(searchLower) - 1;
    }
    
    for (size_t i = 0; i < searchLen; i++) {
        searchLower[i] = tolower(searchTerm[i]);
    }
    searchLower[searchLen] = '\0';
    
    // Search for matches
    for (int i = 0; i < g_gameCount && count < maxEntries; i++) {
        int match = 0;
        
        // Check name
        if (g_games[i].name) {
            char gameName[256];
            size_t nameLen = strlen(g_games[i].name);
            if (nameLen >= sizeof(gameName)) {
                nameLen = sizeof(gameName) - 1;
            }
            
            for (size_t j = 0; j < nameLen; j++) {
                gameName[j] = tolower(g_games[i].name[j]);
            }
            gameName[nameLen] = '\0';
            
            if (strstr(gameName, searchLower)) {
                match = 1;
            }
        }
        
        // Check title
        if (!match && g_games[i].title) {
            char gameTitle[256];
            size_t titleLen = strlen(g_games[i].title);
            if (titleLen >= sizeof(gameTitle)) {
                titleLen = sizeof(gameTitle) - 1;
            }
            
            for (size_t j = 0; j < titleLen; j++) {
                gameTitle[j] = tolower(g_games[i].title[j]);
            }
            gameTitle[titleLen] = '\0';
            
            if (strstr(gameTitle, searchLower)) {
                match = 1;
            }
        }
        
        // Check manufacturer
        if (!match && g_games[i].manufacturer) {
            char gameManufacturer[256];
            size_t mfrLen = strlen(g_games[i].manufacturer);
            if (mfrLen >= sizeof(gameManufacturer)) {
                mfrLen = sizeof(gameManufacturer) - 1;
            }
            
            for (size_t j = 0; j < mfrLen; j++) {
                gameManufacturer[j] = tolower(g_games[i].manufacturer[j]);
            }
            gameManufacturer[mfrLen] = '\0';
            
            if (strstr(gameManufacturer, searchLower)) {
                match = 1;
            }
        }
        
        if (match) {
            outList[count++] = &g_games[i];
        }
    }
    
    return count;
}

// Toggle favorite status
int GameDatabase_ToggleFavorite(const char* name, int isFavorite) {
    if (!g_initialized || !name) {
        return -1;
    }
    
    // Find the game
    int index = -1;
    for (int i = 0; i < g_gameCount; i++) {
        if (g_games[i].name && StrCaseCompare(g_games[i].name, name) == 0) {
            index = i;
            break;
        }
    }
    
    if (index < 0) {
        ROMLoader_DebugLog(LOG_WARNING, "Game not found in database: %s", name);
        return -1;
    }
    
    // Update favorite status
    g_games[index].isFavorite = isFavorite;
    
    // Update flags
    if (isFavorite) {
        g_games[index].flags |= GAME_FLAG_FAVORITE;
    } else {
        g_games[index].flags &= ~GAME_FLAG_FAVORITE;
    }
    
    ROMLoader_DebugLog(LOG_INFO, "%s game in favorites: %s", 
                     isFavorite ? "Added" : "Removed", name);
    
    return 0;
}

// Update last played time
void GameDatabase_UpdateLastPlayed(const char* name) {
    if (!g_initialized || !name) {
        return;
    }
    
    // Find the game
    int index = -1;
    for (int i = 0; i < g_gameCount; i++) {
        if (g_games[i].name && StrCaseCompare(g_games[i].name, name) == 0) {
            index = i;
            break;
        }
    }
    
    if (index < 0) {
        return;
    }
    
    // Update last played time
    g_games[index].lastPlayed = time(NULL);
    g_games[index].playCount++;
    
    // Update flags
    g_games[index].flags |= GAME_FLAG_RECENTLY_PLAYED;
    
    ROMLoader_DebugLog(LOG_INFO, "Updated last played time for game: %s", name);
}

// Compare function for sorting recently played games
static int CompareLastPlayed(const void* a, const void* b) {
    const GameDatabaseEntry* gameA = *(const GameDatabaseEntry**)a;
    const GameDatabaseEntry* gameB = *(const GameDatabaseEntry**)b;
    
    // Sort by last played time (most recent first)
    return (int)(gameB->lastPlayed - gameA->lastPlayed);
}

// Get recently played games
int GameDatabase_GetRecentlyPlayed(GameDatabaseEntry** outList, int maxEntries) {
    if (!g_initialized || !outList || maxEntries <= 0) {
        return -1;
    }
    
    // Get all games with RECENTLY_PLAYED flag
    int count = GameDatabase_GetFilteredList(outList, maxEntries, 
                                          GAME_FLAG_RECENTLY_PLAYED, 0, 
                                          GAME_TYPE_COUNT, COMPATIBILITY_UNKNOWN);
    
    // Sort by last played time
    qsort(outList, count, sizeof(GameDatabaseEntry*), CompareLastPlayed);
    
    return count;
}

// Check if file has a supported ROM extension
static int IsSupportedROMExtension(const char* filename) {
    if (!filename) {
        return 0;
    }
    
    const char* ext = strrchr(filename, '.');
    if (!ext) {
        return 0;
    }
    
    // Convert to lowercase
    char extLower[16];
    size_t extLen = strlen(ext);
    if (extLen >= sizeof(extLower)) {
        extLen = sizeof(extLower) - 1;
    }
    
    for (size_t i = 0; i < extLen; i++) {
        extLower[i] = tolower(ext[i]);
    }
    extLower[extLen] = '\0';
    
    // Check against supported extensions
    const char* supportedExts[] = {
        ".zip", ".7z", ".rar", ".bin", ".rom", ".iso"
    };
    
    for (int i = 0; i < sizeof(supportedExts) / sizeof(supportedExts[0]); i++) {
        if (strcmp(extLower, supportedExts[i]) == 0) {
            return 1;
        }
    }
    
    return 0;
}

// Extract ROM name from filename
static void ExtractROMName(const char* filename, char* romName, int maxLen) {
    if (!filename || !romName || maxLen <= 0) {
        return;
    }
    
    // Get the filename part (remove path)
    const char* baseName = strrchr(filename, '/');
    if (!baseName) {
        baseName = filename;
    } else {
        baseName++; // Skip the '/'
    }
    
    // Remove extension
    size_t nameLen = strlen(baseName);
    const char* ext = strrchr(baseName, '.');
    if (ext) {
        nameLen = ext - baseName;
    }
    
    // Copy to output buffer (with length limit)
    if (nameLen >= (size_t)maxLen) {
        nameLen = maxLen - 1;
    }
    strncpy(romName, baseName, nameLen);
    romName[nameLen] = '\0';
}

// Scan directory for ROMs
int GameDatabase_ScanDirectory(const char* directory) {
    if (!g_initialized || !directory) {
        return -1;
    }
    
    DIR* dir = opendir(directory);
    if (!dir) {
        ROMLoader_DebugLog(LOG_ERROR, "Failed to open directory: %s", directory);
        return -1;
    }
    
    int count = 0;
    struct dirent* entry;
    
    ROMLoader_DebugLog(LOG_INFO, "Scanning directory for ROMs: %s", directory);
    
    while ((entry = readdir(dir)) != NULL) {
        // Skip . and ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // Check if it's a regular file
        char fullPath[1024];
        snprintf(fullPath, sizeof(fullPath), "%s/%s", directory, entry->d_name);
        
        struct stat st;
        if (stat(fullPath, &st) != 0 || !S_ISREG(st.st_mode)) {
            continue;
        }
        
        // Check if it's a ROM file
        if (IsSupportedROMExtension(entry->d_name)) {
            // Extract ROM name
            char romName[256];
            ExtractROMName(entry->d_name, romName, sizeof(romName));
            
            // Check if ROM is already in database
            int exists = 0;
            for (int i = 0; i < g_gameCount; i++) {
                if (g_games[i].name && StrCaseCompare(g_games[i].name, romName) == 0) {
                    // Update path
                    free((void*)g_games[i].path);
                    g_games[i].path = StrDup(fullPath);
                    exists = 1;
                    break;
                }
            }
            
            // Add new ROM to database
            if (!exists) {
                GameDatabaseEntry newEntry;
                memset(&newEntry, 0, sizeof(newEntry));
                
                // Basic information
                newEntry.name = StrDup(romName);
                newEntry.title = StrDup(romName); // Default title same as name
                newEntry.path = StrDup(fullPath);
                newEntry.flags = GAME_FLAG_WORKING; // Assume working by default
                newEntry.compatibility = COMPATIBILITY_UNKNOWN;
                newEntry.type = GAME_TYPE_ARCADE; // Default to arcade
                
                // Add to database
                if (GameDatabase_AddGame(&newEntry) == 0) {
                    count++;
                } else {
                    // Free strings on error
                    free((void*)newEntry.name);
                    free((void*)newEntry.title);
                    free((void*)newEntry.path);
                }
            }
        }
    }
    
    closedir(dir);
    
    ROMLoader_TrackLoadStep("DATABASE INIT", "Scanned %s: found %d new ROMs", directory, count);
    
    return count;
}

// Update compatibility rating
void GameDatabase_UpdateCompatibility(const char* name, GameCompatibility rating) {
    if (!g_initialized || !name) {
        return;
    }
    
    // Find the game
    int index = -1;
    for (int i = 0; i < g_gameCount; i++) {
        if (g_games[i].name && StrCaseCompare(g_games[i].name, name) == 0) {
            index = i;
            break;
        }
    }
    
    if (index < 0) {
        return;
    }
    
    // Update compatibility rating
    g_games[index].compatibility = rating;
    
    // Update flags based on compatibility
    if (rating == COMPATIBILITY_NONE) {
        g_games[index].flags &= ~GAME_FLAG_WORKING;
        g_games[index].flags |= GAME_FLAG_NOT_WORKING;
    } else if (rating >= COMPATIBILITY_GOOD) {
        g_games[index].flags |= GAME_FLAG_WORKING;
        g_games[index].flags &= ~GAME_FLAG_NOT_WORKING;
    }
    
    ROMLoader_DebugLog(LOG_INFO, "Updated compatibility rating for %s: %d", name, rating);
}

// Set user rating
void GameDatabase_SetRating(const char* name, float rating) {
    if (!g_initialized || !name) {
        return;
    }
    
    // Clamp rating to 0-5 range
    if (rating < 0.0f) rating = 0.0f;
    if (rating > 5.0f) rating = 5.0f;
    
    // Find the game
    int index = -1;
    for (int i = 0; i < g_gameCount; i++) {
        if (g_games[i].name && StrCaseCompare(g_games[i].name, name) == 0) {
            index = i;
            break;
        }
    }
    
    if (index < 0) {
        return;
    }
    
    // Update rating
    g_games[index].rating = rating;
    
    ROMLoader_DebugLog(LOG_INFO, "Updated user rating for %s: %.1f", name, rating);
}

// Compare function for sorting favorites
static int CompareFavoriteRating(const void* a, const void* b) {
    const GameDatabaseEntry* gameA = *(const GameDatabaseEntry**)a;
    const GameDatabaseEntry* gameB = *(const GameDatabaseEntry**)b;
    
    // Sort by rating (highest first)
    if (gameA->rating > gameB->rating) return -1;
    if (gameA->rating < gameB->rating) return 1;
    
    // If ratings are equal, sort by name
    return StrCaseCompare(gameA->name, gameB->name);
}

// Get favorites
int GameDatabase_GetFavorites(GameDatabaseEntry** outList, int maxEntries) {
    if (!g_initialized || !outList || maxEntries <= 0) {
        return -1;
    }
    
    // Get all games with FAVORITE flag
    int count = GameDatabase_GetFilteredList(outList, maxEntries, 
                                          GAME_FLAG_FAVORITE, 0, 
                                          GAME_TYPE_COUNT, COMPATIBILITY_UNKNOWN);
    
    // Sort by rating
    qsort(outList, count, sizeof(GameDatabaseEntry*), CompareFavoriteRating);
    
    return count;
} 