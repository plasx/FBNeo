#include "burnint.h"
#include "konami_intf.h"

// Stub implementation for Metal build
#ifdef METAL_BUILD

void konami_init() {}
void konami_reset() {}
void konami_exit() {}
int konami_execute(INT32 cycles) { return 0; }
void konami_set_irq_line(INT32 irqline, INT32 state) {}
int konami_get_pc() { return 0; }
void konami_set_context(void *reg) {}
void konami_get_context(void *reg) {}
void konami_state_save(void *file) {}
void konami_state_load(void *file) {}
void konami_scan(INT32 nAction) {}

#endif 