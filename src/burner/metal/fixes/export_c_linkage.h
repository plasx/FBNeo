#pragma once

// Basic type definitions for compatibility
typedef char TCHAR;
typedef int INT32;

#ifndef EXPORT_C_LINKAGE_H
#define EXPORT_C_LINKAGE_H

#include "burnint.h"

#ifdef __cplusplus
extern "C" {
#endif

// Z80 bridge functions for C linkage
void Z80StopExecute_Bridge();
INT32 z80TotalCycles_Bridge();

// QSound functions with C linkage
void QsndSyncZ80();

// BurnState functions
extern int BurnStateLoad(char* szName, int nOffset, int (*pLoadGame)());
extern int BurnStateSave(char* szName, int nOffset);

// TMS34010 functions
extern int tms34010_generate_scanline(int line, int (*callback)(int, struct _tms34010_display_params*));

// QSound functions
extern int QsndScan(int nAction);

// BurnSample functions
extern void BurnSampleRender_INT(unsigned int nSegmentLength);

// String conversion functions
extern void TCHARToANSI(const char* pszInString, char* pszOutString, int nOutSize);

// 68K functions
extern void m68k_modify_timeslice(int value);

#ifdef __cplusplus
}
#endif

// Declaration outside of extern "C" block to match burn.h
extern TCHAR szAppEEPROMPath[260]; // MAX_PATH is 260

#endif // EXPORT_C_LINKAGE_H 