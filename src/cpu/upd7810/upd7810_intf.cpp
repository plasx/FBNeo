#include "burnint.h"
#include "upd7810_intf.h"

// Stub implementation for Metal build
#ifdef METAL_BUILD

void upd7810_init() {}
void upd7810_reset() {}
void upd7810_exit() {}
INT32 upd7810_execute(INT32 cycles) { return 0; }
void upd7810_set_irq_line(INT32 irqline, INT32 state) {}
INT32 upd7810_get_pc() { return 0; }
void upd7810_state_save(void *file) {}
void upd7810_state_load(void *file) {}
void upd7810_scan(INT32 nAction) {}

#endif 