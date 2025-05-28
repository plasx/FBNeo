#ifndef METAL_SAVESTATE_STUBS_H
#define METAL_SAVESTATE_STUBS_H

#ifdef __cplusplus
extern "C" {
#endif

// Basic type definitions if needed
typedef int INT32;

// Function declarations
INT32 Metal_SaveState(INT32 nSlot);
INT32 Metal_LoadState(INT32 nSlot);
INT32 Metal_InitSaveState();
INT32 Metal_ExitSaveState();
INT32 Metal_QuickSave();
INT32 Metal_QuickLoad();
INT32 Metal_SetSaveSlot(INT32 nSlot);
INT32 Metal_ListSaveStates(int* slots, int maxSlots);
INT32 Metal_DeleteSaveState(int slot);
bool Metal_SaveStateExists(int slot);
const char* Metal_GetSaveStateStatus();
int Metal_GetCurrentSaveSlot();

#ifdef __cplusplus
}
#endif

#endif // METAL_SAVESTATE_STUBS_H 