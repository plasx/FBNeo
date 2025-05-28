#include "burnint.h"
#include "z180_intf.h"

// Stub implementation for Metal build
#ifdef METAL_BUILD

// Z180 specific
void z180_init() {}
void z180_reset() {}
void z180_exit() {}
INT32 z180_execute(INT32 cycles) { return 0; }
void z180_set_irq_line(INT32 irqline, INT32 state) {}
INT32 z180_get_pc() { return 0; }
INT32 z180_get_sp() { return 0; }
void z180_set_pc(UINT32 val) {}
void z180_set_nmi_line(INT32 state) {}
void z180_set_irq_hold() {}
void z180_set_irq_callback(INT32 (*callback)(INT32)) {}
INT32 z180_total_cycles() { return 0; }
void z180_new_frame() {}
INT32 z180_idle(INT32 cycles) { return 0; }

// Memory handlers
void z180_write(INT32 address, UINT8 data) {}
void z180_write_rom(INT32 address, UINT8 data) {}
UINT8 z180_read(INT32 address) { return 0; }
void z180_write_port(INT32 port, UINT8 data) {}
UINT8 z180_read_port(INT32 port) { return 0; }

// Debugging
INT32 z180_segmentbase(INT32 num) { return 0; }
INT32 z180_segment(INT32 addr) { return 0; }
void z180_state_save(void *file) {}
void z180_state_load(void *file) {}
void z180_scan(INT32 nAction) {}

#endif 