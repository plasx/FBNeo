#include "rom_verify.h"
#include "debug_controller.h"
#include <stdbool.h>
bool ROM_Verify(const char* romPath) { Debug_PrintSectionHeader(0, "Located ROM: %s", romPath); return true; }
int Metal_DumpZipContents(const char* zipPath) { return 0; }
bool VerifyCRCForMvsC(const char* zipPath) { return true; }
