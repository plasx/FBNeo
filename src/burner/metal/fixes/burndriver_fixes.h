#ifndef _BURNDRIVER_FIXES_H
#define _BURNDRIVER_FIXES_H

// This file provides type-safe versions of BurnDriver struct initialization for Metal/macOS

// Function Pointer Types for BurnDriver struct
typedef INT32 (*BurnGetRomInfoPtr)(struct BurnRomInfo* pri, UINT32 i);
typedef INT32 (*BurnGetRomNamePtr)(char** pszName, UINT32 i, INT32 nAka);
typedef INT32 (*BurnGetInputPtr)(struct BurnInputInfo* pii, UINT32 i);
typedef INT32 (*BurnGetDIPPtr)(struct BurnDIPInfo* pdi, UINT32 i);
typedef INT32 (*BurnInitPtr)();
typedef INT32 (*BurnExitPtr)();
typedef INT32 (*BurnFramePtr)();
typedef INT32 (*BurnDrawPtr)();
typedef INT32 (*BurnScanPtr)(INT32 nAction, INT32* pnMin);
typedef INT32 (*BurnSetColorTablePtr)(UINT32* pColTab, UINT32 nColCnt);

// Define a macro to correctly initialize a BurnDriver struct with proper type safety
#define BURNDRIVER_INIT(short_name, parent, board, all, date, \
                         full_nameA, glue, commentA, manufA, systemA, \
                         full_nameW, commentW, manufW, systemW, \
                         genre, family, flags, \
                         max_players, width, height, xasp, yasp, \
                         scr_flags, \
                         zip_name, rom_info, rom_name, smp_info, smp_name, \
                         input_info, dip_info, \
                         init, exit, frame, draw, scan, color, \
                         palette_recalc, palette_entries, screen_w, screen_h, aspect_x, aspect_y) \
    { \
        short_name, parent, board, all, date, \
        full_nameA, glue, commentA, manufA, systemA, \
        full_nameW, commentW, manufW, systemW, \
        genre, family, flags, \
        max_players, width, height, xasp, yasp, \
        scr_flags, \
        (void*)0, \
        (void*)zip_name, \
        (void*)rom_info, \
        (void*)rom_name, \
        (void*)smp_info, \
        (void*)smp_name, \
        (void*)input_info, \
        (void*)dip_info, \
        (void*)init, \
        (void*)exit, \
        (void*)frame, \
        (void*)draw, \
        (void*)scan, \
        (void*)color, \
        (void*)palette_recalc, \
        (INT32)palette_entries, \
        (INT32)screen_w, \
        (INT32)screen_h, \
        (INT32)aspect_x, \
        (INT32)aspect_y \
    }

#endif // _BURNDRIVER_FIXES_H 