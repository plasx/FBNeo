#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Define types we need
typedef int INT32;
typedef short INT16;
typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int UINT32;

// YM2151 sound interface stubs
INT32 BurnYM2151Init(INT32 nClockFrequency) {
    printf("[SOUND] BurnYM2151Init(%d)\n", nClockFrequency);
    return 0;
}

void BurnYM2151SetRoute(INT32 nIndex, double nVolume, INT32 nRouteDir) {
    printf("[SOUND] BurnYM2151SetRoute(%d, %f, %d)\n", nIndex, nVolume, nRouteDir);
}

void BurnYM2151Reset() {
    printf("[SOUND] BurnYM2151Reset()\n");
}

void BurnYM2151Exit() {
    printf("[SOUND] BurnYM2151Exit()\n");
}

void BurnYM2151Render(INT16* pSoundBuf, INT32 nSegmentLength) {
    // Just clear buffer to silence
    if (pSoundBuf && nSegmentLength > 0) {
        memset(pSoundBuf, 0, nSegmentLength * 2 * sizeof(INT16));
    }
}

UINT32 BurnYM2151Read(UINT32 nAddress) {
    return 0;
}

void BurnYM2151Write(UINT32 nAddress, UINT32 nData) {
}

// MSM6295 stubs
INT32 MSM6295Init(INT32 nChip, INT32 nSamplerate, INT32 bAddSignal, INT32 nSampleSize, INT32 nOutputDir, double nVolume) {
    printf("[SOUND] MSM6295Init(%d, %d)\n", nChip, nSamplerate);
    return 0;
}

void MSM6295SetRoute(INT32 nChip, double nVolume, INT32 nRouteDir) {
    printf("[SOUND] MSM6295SetRoute(%d, %f, %d)\n", nChip, nVolume, nRouteDir);
}

void MSM6295Reset(INT32 nChip) {
    printf("[SOUND] MSM6295Reset(%d)\n", nChip);
}

void MSM6295Exit(INT32 nChip) {
    printf("[SOUND] MSM6295Exit(%d)\n", nChip);
}

INT32 MSM6295Render(INT32 nChip, INT16* pSoundBuf, INT32 nSegmentLength) {
    // Just clear buffer to silence
    if (pSoundBuf && nSegmentLength > 0) {
        memset(pSoundBuf, 0, nSegmentLength * 2 * sizeof(INT16));
    }
    return 0;
}

void MSM6295Command(INT32 nChip, UINT8 nCommand) {
}

INT32 MSM6295ReadStatus(INT32 nChip) {
    return 0;
}

INT32 MSM6295Scan(INT32 nAction, INT32* pnMin) {
    return 0;
}

// EEPROM stubs
void EEPROMInit(INT32 nSize, INT32 nEraseValue) {
    printf("[SOUND] EEPROMInit(%d, %d)\n", nSize, nEraseValue);
}

void EEPROMExit() {
    printf("[SOUND] EEPROMExit()\n");
}

void EEPROMReset() {
    printf("[SOUND] EEPROMReset()\n");
}

void EEPROMScan(INT32 nAction, INT32* pnMin) {
}

INT32 EEPROMAvailable() {
    return 1; // Always available
}

// RF5C68 stubs
void RF5C68PCMReset() {
    printf("[SOUND] RF5C68PCMReset()\n");
}

void RF5C68PCMInit(INT32 clock) {
    printf("[SOUND] RF5C68PCMInit(%d)\n", clock);
}

void RF5C68PCMExit() {
    printf("[SOUND] RF5C68PCMExit()\n");
}

void RF5C68PCMUpdate(INT16* pBuf, INT32 samples) {
    // Just clear buffer to silence
    if (pBuf && samples > 0) {
        memset(pBuf, 0, samples * 2 * sizeof(INT16));
    }
}

void RF5C68PCMRegWrite(UINT8 offset, UINT8 data) {
}

// 2151 interface stubs
void YM2151WriteReg(INT32 nDevice, INT32 nReg, UINT8 nValue) {
}

UINT8 YM2151ReadStatus(INT32 nDevice) {
    return 0;
}

void YM2151IRQHandler(INT32 nDevice, INT32 nIRQ) {
} 