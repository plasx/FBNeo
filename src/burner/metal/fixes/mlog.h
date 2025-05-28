#pragma once

#include <stdio.h>

// Simple logging macro that forwards to printf
#define MLog(format, ...) printf(format, ##__VA_ARGS__)

// Alternative implementation (uncomment if you want to disable logging):
// #define MLog(format, ...) ((void)0) 