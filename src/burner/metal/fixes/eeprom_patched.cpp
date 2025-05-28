// Patched version of eeprom.cpp for Metal build

#include "burnint.h"
#include <string.h>
#include <stdio.h>

// Replace BurnMalloc/BurnFree with standard malloc/free
#define BurnMalloc malloc
#define BurnFree free

#ifndef MAX_EEPROM_SIZE
#define MAX_EEPROM_SIZE 0x10000
#endif

// EEPROM constants
#define EEPROM_CLEAR_LINE  0
#define EEPROM_ASSERT_LINE 1
#define EEPROM_PULSE_LINE  2

// Debug variables
INT32 Debug_EEPROMInitted = 0;

// EEPROM structure
struct _eeprom_interface
{
	INT32 address_bits;          // EEPROM has 2^address_bits cells
	INT32 data_bits;             // every cell has this many bits (8 or 16)
	const char *cmd_read;        // command for reading (default 110)
	const char *cmd_write;       // command for writing (default 101)
	const char *cmd_erase;       // command for erasing (default 111)
	const char *cmd_lock;        // command for write-locking
	const char *cmd_unlock;      // command for removing the write-lock
	INT32 enable_multi_read;    // set to 1 to enable multiple values to be read from a single read command
	INT32 reset_delay;          // number of times eeprom_read_bit() should return 0 after a reset,
								// before starting to return 1.
	// Some pc42xs chips have 9 bit words. To simulate this, write a 0 in bit 7,
	// while writing the 8 bit data to bits 8-15 (reversed)
	INT32 enable_0_to_7_write;
	const char* write_ptr;
	UINT32 data_size;
	UINT8* data;
	INT32 serial_mode;
	INT32 reset_line;
	INT32 clock_line;
	INT32 write_line;
	INT32 reset_delay_ramp;
	INT32 locked;
};

// EEPROM interface
static struct _eeprom_interface* eeprom_intf;

// EEPROM initialization
void EEPROMInit(const struct _eeprom_interface* interface)
{
	if (eeprom_intf == NULL) {
		eeprom_intf = (struct _eeprom_interface*)BurnMalloc(sizeof(struct _eeprom_interface));
		memset(eeprom_intf, 0, sizeof(struct _eeprom_interface));
	}
	
	*eeprom_intf = *interface;

	if (eeprom_intf->data) {
		BurnFree(eeprom_intf->data);
	}
	
	eeprom_intf->data = (UINT8*)BurnMalloc(eeprom_intf->data_size);
	memset(eeprom_intf->data, 0xff, eeprom_intf->data_size);
	
	eeprom_intf->reset_line = EEPROM_ASSERT_LINE;
	eeprom_intf->clock_line = EEPROM_ASSERT_LINE;
	eeprom_intf->write_line = EEPROM_ASSERT_LINE;
	
	Debug_EEPROMInitted = 1;
}

// EEPROM exit
void EEPROMExit()
{
	Debug_EEPROMInitted = 0;
	
	if (eeprom_intf) {
		if (eeprom_intf->data) {
			BurnFree(eeprom_intf->data);
		}
		BurnFree(eeprom_intf);
		eeprom_intf = NULL;
	}
}

// Set the default EEPROM size
void EEPROMSetDefaultData(const UINT8* data, INT32 size)
{
	if (eeprom_intf && eeprom_intf->data) {
		memcpy(eeprom_intf->data, data, size);
	}
}

// Get the EEPROM data
void* EEPROMGetData()
{
	if (eeprom_intf) {
		return eeprom_intf->data;
	}
	return NULL;
}

// Get the EEPROM size
INT32 EEPROMGetSize()
{
	if (eeprom_intf) {
		return eeprom_intf->data_size;
	}
	return 0;
}

// Set the EEPROM reset line state
void EEPROMSetCSLine(INT32 state)
{
	if (eeprom_intf) {
		eeprom_intf->reset_line = state;
	}
}

// Set the EEPROM clock line state
void EEPROMSetClockLine(INT32 state)
{
	if (eeprom_intf) {
		eeprom_intf->clock_line = state;
	}
}

// Set the EEPROM write line state
void EEPROMSetOutputEnable(INT32 state)
{
	// Not used in this implementation
}

// Write a bit to the EEPROM
void EEPROMWriteBit(INT32 bit)
{
	if (eeprom_intf) {
		eeprom_intf->write_line = bit;
	}
}

// Read a bit from the EEPROM
INT32 EEPROMRead()
{
	if (!eeprom_intf) {
		return 0;
	}
	
	// Simple stub implementation
	return 0;
}

// Toggle the latch - not implemented for Metal build
void EEPROMToggleEraseWrite()
{
	// Not used in this implementation
}

// Scan the EEPROM state
INT32 EEPROMScan(INT32 nAction, INT32* pnMin)
{
	if (!eeprom_intf) return 0;
	
	struct BurnArea ba;
	
	if (nAction & ACB_DRIVER_DATA) {
		if (pnMin && *pnMin < 0x020902) {
			*pnMin = 0x020902;
		}
		
		ba.Data = eeprom_intf->data;
		ba.nLen = eeprom_intf->data_size;
		ba.nAddress = 0;
		ba.szName = "EEPROM Data";
		
		// Use the function pointer declaration from burnint.h
		if (BurnAcb) {
			BurnAcb(&ba);
		}
	}
	
	return 0;
}

extern "C" {
	// Add these to make them accessible to the driver
	void EEPROMSetClockLine_Metal(INT32 state) {
		EEPROMSetClockLine(state);
	}
	
	void EEPROMSetCSLine_Metal(INT32 state) {
		EEPROMSetCSLine(state);
	}
	
	void EEPROMWriteBit_Metal(INT32 bit) {
		EEPROMWriteBit(bit);
	}
	
	UINT8 EEPROMRead_Metal() {
		return EEPROMRead();
	}
} 