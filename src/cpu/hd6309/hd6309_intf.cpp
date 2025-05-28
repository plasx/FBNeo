#include "burnint.h"
#include "hd6309_intf.h"

// Stub implementation for Metal build
#ifdef METAL_BUILD

void hd6309_init() {}
void hd6309_reset() {}
void hd6309_exit() {}
int hd6309_execute(INT32 cycles) { return 0; }
void hd6309_set_irq_line(INT32 irqline, INT32 state) {}
int hd6309_get_pc() { return 0; }
void hd6309_set_context(void *reg) {}
void hd6309_get_context(void *reg) {}
void hd6309_state_save(void *file) {}
void hd6309_state_load(void *file) {}
void hd6309_scan(INT32 nAction) {}

#endif 