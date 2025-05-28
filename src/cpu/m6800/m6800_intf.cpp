#include "burnint.h"
#include "m6800_intf.h"

// Stub implementation for Metal build
#ifdef METAL_BUILD

// M6800 specific
void m6800_init() {}
void m6800_reset() {}
void m6800_exit() {}
int m6800_execute(INT32 cycles) { return 0; }
void m6800_set_irq_line(INT32 irqline, INT32 state) {}
int m6800_get_pc() { return 0; }
void m6800_set_context(void *reg) {}
void m6800_get_context(void *reg) {}
void m6800_state_save(void *file) {}
void m6800_state_load(void *file) {}
void m6800_scan(INT32 nAction) {}

// M6801 specific
void m6801_init() {}
void m6801_reset() {}
void m6801_exit() {}
int m6801_execute(INT32 cycles) { return 0; }
void m6801_set_irq_line(INT32 irqline, INT32 state) {}
int m6801_get_pc() { return 0; }
void m6801_set_context(void *reg) {}
void m6801_get_context(void *reg) {}
void m6801_state_save(void *file) {}
void m6801_state_load(void *file) {}
void m6801_scan(INT32 nAction) {}

// M6802 specific
void m6802_init() {}
void m6802_reset() {}
void m6802_exit() {}
int m6802_execute(INT32 cycles) { return 0; }
void m6802_set_irq_line(INT32 irqline, INT32 state) {}
int m6802_get_pc() { return 0; }
void m6802_set_context(void *reg) {}
void m6802_get_context(void *reg) {}
void m6802_state_save(void *file) {}
void m6802_state_load(void *file) {}
void m6802_scan(INT32 nAction) {}

// M6803 specific
void m6803_init() {}
void m6803_reset() {}
void m6803_exit() {}
int m6803_execute(INT32 cycles) { return 0; }
void m6803_set_irq_line(INT32 irqline, INT32 state) {}
int m6803_get_pc() { return 0; }
void m6803_set_context(void *reg) {}
void m6803_get_context(void *reg) {}
void m6803_state_save(void *file) {}
void m6803_state_load(void *file) {}
void m6803_scan(INT32 nAction) {}

// M6808 specific
void m6808_init() {}
void m6808_reset() {}
void m6808_exit() {}
int m6808_execute(INT32 cycles) { return 0; }
void m6808_set_irq_line(INT32 irqline, INT32 state) {}
int m6808_get_pc() { return 0; }
void m6808_set_context(void *reg) {}
void m6808_get_context(void *reg) {}
void m6808_state_save(void *file) {}
void m6808_state_load(void *file) {}
void m6808_scan(INT32 nAction) {}

// HD63701 specific
void hd63701_init() {}
void hd63701_reset() {}
void hd63701_exit() {}
int hd63701_execute(INT32 cycles) { return 0; }
void hd63701_set_irq_line(INT32 irqline, INT32 state) {}
int hd63701_get_pc() { return 0; }
void hd63701_set_context(void *reg) {}
void hd63701_get_context(void *reg) {}
void hd63701_state_save(void *file) {}
void hd63701_state_load(void *file) {}
void hd63701_scan(INT32 nAction) {}

// NSC8105 specific
void nsc8105_init() {}
void nsc8105_reset() {}
void nsc8105_exit() {}
int nsc8105_execute(INT32 cycles) { return 0; }
void nsc8105_set_irq_line(INT32 irqline, INT32 state) {}
int nsc8105_get_pc() { return 0; }
void nsc8105_set_context(void *reg) {}
void nsc8105_get_context(void *reg) {}
void nsc8105_state_save(void *file) {}
void nsc8105_state_load(void *file) {}
void nsc8105_scan(INT32 nAction) {}

#endif 