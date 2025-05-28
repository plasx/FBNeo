#include "burnint.h"
#include "i8051_intf.h"

// Stub implementation for Metal build
#ifdef METAL_BUILD

void i8051_init() {}
void i8051_reset() {}
void i8051_exit() {}
INT32 i8051_execute(INT32 cycles) { return 0; }
void i8051_set_irq_line(INT32 irqline, INT32 state) {}
INT32 i8051_get_pc() { return 0; }
void i8051_state_save(void *file) {}
void i8051_state_load(void *file) {}
void i8051_scan(INT32 nAction) {}

#endif 