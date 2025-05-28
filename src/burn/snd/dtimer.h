// Stub dtimer.h for Metal build
#ifndef _DTIMER_H_
#define _DTIMER_H_

// Stub timer functionality for Metal build
#ifdef USE_METAL_FIXES

// Forward declaration
class dtimer;

// Timer stub functions
static inline void timer_enable(int, int) {}
static inline void timer_set(int, double) {}
static inline void timer_adjust_oneshot(int, double) {}
static inline void timer_reset(int, double) {}
static inline double timer_get_time() { return 0.0; }

// Additional timer functions needed by k054539.cpp
static inline void timerInit() {}
static inline void timerExit() {}
static inline void timerScan() {}
static inline void timerAdd(dtimer&) {}  // Fixed: accept dtimer reference

// dtimer class stub for k054539.cpp
class dtimer {
public:
    int timer_param = 0;  // Add timer_param member
    
    void init(int, void (*)(int)) {}
    void start(int, int, int, int) {}
    void stop_retrig() {}
    int isrunning() { return 0; }  // Add isrunning method
    void run(int) {}  // Add run method  
    void scan() {}  // Add scan method
};

// Timer constants
#define MAX_TIMER 8
#define TIMER_ONE_SHOT 1
#define TIMER_REPEATING 0

#endif // USE_METAL_FIXES

#endif // _DTIMER_H_ 