#include "burnint.h"
#include "tms34_intf.h"

// Stub implementation for Metal build
#ifdef METAL_BUILD

void tms34010_init() {}
void tms34010_reset() {}
void tms34010_exit() {}
int tms34010_execute(int cycles) { return 0; }
void tms34010_set_irq_line(int irqline, int state) {}
int tms34010_get_pc() { return 0; }
void tms34010_set_pixel_read_function(pixel_read_handler handler) {}
void tms34010_set_pixel_write_function(pixel_write_handler handler) {}
void tms34010_set_scanline_callback(scanline_callback callback) {}
int tms34010_io_display_blanked() { return 0; }
void tms34010_state_save(void *file) {}
void tms34010_state_load(void *file) {}
void tms34010_scan(INT32 nAction) {}

#endif 