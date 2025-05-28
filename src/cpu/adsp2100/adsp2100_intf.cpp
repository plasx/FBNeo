#include "burnint.h"
#include "adsp2100_intf.h"

// Stub implementation for Metal build
#ifdef METAL_BUILD

void adsp21xx_init() {}
void adsp21xx_reset() {}
void adsp21xx_exit() {}
int adsp21xx_execute(INT32 cycles) { return 0; }
void adsp21xx_set_irq_line(INT32 irqline, INT32 state) {}
int adsp21xx_get_pc() { return 0; }
void adsp21xx_state_save(void *file) {}
void adsp21xx_state_load(void *file) {}
void adsp21xx_scan(INT32 nAction) {}

#endif 