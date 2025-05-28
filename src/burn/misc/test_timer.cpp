#include "../timer.h"
#include <stdio.h>

// Test program to verify that the cpu_core_config stub works

int test_run(int cycles) {
    printf("Run called with %d cycles\n", cycles);
    return cycles;
}

int test_totalcycles() {
    return 0;
}

void test_runend() {
    printf("RunEnd called\n");
}

int main() {
    cpu_core_config test_config;
    test_config.run = test_run;
    test_config.totalcycles = test_totalcycles;
    test_config.runend = test_runend;
    
    // Test BurnTimerAttach
    BurnTimerAttach(&test_config, 1000000);
    
    printf("Timer test successful\n");
    return 0;
} 