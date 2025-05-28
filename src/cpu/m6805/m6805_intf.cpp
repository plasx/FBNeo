#include "burnint.h"
#include "m6805_intf.h"

// Stub implementation for Metal build
#ifdef METAL_BUILD

void m6805_init() {}
void m6805_reset() {}
void m6805_exit() {}
int m6805_execute(int cycles) { return 0; }
void m6805_set_irq_line(int irqline, int state) {}
int m6805_get_pc() { return 0; }
void m6805_state_save(void *file) {}
void m6805_state_load(void *file) {}
void m6805_scan(INT32 nAction) {}

// HD63705
void hd63705_init() {}
void hd63705_reset() {}
void hd63705_exit() {}
int hd63705_execute(int cycles) { return 0; }
void hd63705_set_irq_line(int irqline, int state) {}
int hd63705_get_pc() { return 0; }
void hd63705_state_save(void *file) {}
void hd63705_state_load(void *file) {}
void hd63705_scan(INT32 nAction) {}

#endif 