#include "burnint.h"

// Replacement for interface.cpp functions with Unicode issues

// Define our own version of tcsncpy for Metal builds
void strncpy_metal(char* dest, const char* src, int maxLen) {
    if (dest && src) {
        strncpy(dest, src, maxLen);
        dest[maxLen - 1] = '\0'; // Ensure null-termination
    }
}

// This is a simplified version of IntInfoFree from interface.cpp
void IntInfoFree(InterfaceInfo* pInfo) {
    if (pInfo == NULL) {
        return;
    }

    for (int i = 0; i < INTERFACE_MAX_SETTINGS; i++) {
        if (pInfo->ppszInterfaceSettings[i]) {
            free(pInfo->ppszInterfaceSettings[i]);
            pInfo->ppszInterfaceSettings[i] = NULL;
        }
    }

    for (int i = 0; i < INTERFACE_MAX_SETTINGS; i++) {
        if (pInfo->ppszModuleSettings[i]) {
            free(pInfo->ppszModuleSettings[i]);
            pInfo->ppszModuleSettings[i] = NULL;
        }
    }
}

// This is a replacement for IntInfoAddStringInterface from interface.cpp
int IntInfoAddStringInterface(InterfaceInfo* pInfo, TCHAR* szString) {
    if (pInfo->nInterfaceSettings >= INTERFACE_MAX_SETTINGS) {
        return 1;
    }

    if (pInfo->ppszInterfaceSettings[pInfo->nInterfaceSettings] == NULL) {
        pInfo->ppszInterfaceSettings[pInfo->nInterfaceSettings] = (TCHAR*)malloc(MAX_PATH * sizeof(TCHAR));
        if (pInfo->ppszInterfaceSettings[pInfo->nInterfaceSettings] == NULL) {
            return 1;
        }
    }

    strncpy_metal(pInfo->ppszInterfaceSettings[pInfo->nInterfaceSettings], szString, MAX_PATH);
    pInfo->nInterfaceSettings++;

    return 0;
}

// This is a replacement for IntInfoAddStringModule from interface.cpp
int IntInfoAddStringModule(InterfaceInfo* pInfo, TCHAR* szString) {
    if (pInfo->nModuleSettings >= INTERFACE_MAX_SETTINGS) {
        return 1;
    }

    if (pInfo->ppszModuleSettings[pInfo->nModuleSettings] == NULL) {
        pInfo->ppszModuleSettings[pInfo->nModuleSettings] = (TCHAR*)malloc(MAX_PATH * sizeof(TCHAR));
        if (pInfo->ppszModuleSettings[pInfo->nModuleSettings] == NULL) {
            return 1;
        }
    }

    strncpy_metal(pInfo->ppszModuleSettings[pInfo->nModuleSettings], szString, MAX_PATH);
    pInfo->nModuleSettings++;

    return 0;
}

// Minimal implementation of interface functions
int InterfaceInfo(InterfaceInfo* pInfo, const TCHAR* pszModuleName) {
    if (pInfo == NULL) {
        return 1;
    }

    // Reset the interface info structure
    memset(pInfo, 0, sizeof(InterfaceInfo));
    
    // Return success
    return 0;
}

int InterfaceExit() {
    return 0;
}
