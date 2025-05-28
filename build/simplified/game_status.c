#include <stdio.h>
#include <string.h>
#include <stdbool.h>

char g_gameTitle[256] = "FBNeo Metal";
bool g_gameRunning = false;

const char* Metal_GetGameTitle(void) {
    return g_gameTitle;
}

void Metal_SetGameTitle(const char* title) {
    if (title) {
        strncpy(g_gameTitle, title, sizeof(g_gameTitle) - 1);
    }
}

bool Metal_IsGameRunning(void) {
    return g_gameRunning;
}

void Metal_SetGameRunning(bool running) {
    g_gameRunning = running;
}

float Metal_GetFrameRate(void) {
    return 60.0f;
}

int Metal_GetTotalFrames(void) {
    return 0;
}
