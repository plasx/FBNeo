// Patched version of burn_led.cpp for Metal build
// NOTE: This file is compiled without the c_cpp_fixes.h header!

#include "burnint.h"
#include <stdio.h>
#include <string.h>

// Debug variables
INT32 Debug_BurnLedInitted = 0;

// Define the LED info structures
struct BurnLedInfo {
	char text[16];
	INT32 status;
	INT32 xpos;
	INT32 ypos;
	INT32 xspa;
	INT32 yspa;
	INT32 xadj;
	INT32 yadj;
	INT32 mask;
	INT32 lamp;
};

struct BurnLedBlendInfo {
	INT32 solid_color;
	UINT32 lamp_color;
	INT32 alpha;
	INT32 transparency;
	INT32 transparency_alpha;
	void* surface;
	INT32 width;
	INT32 height;
	INT32 pitch;
	INT32 bpp;
};

// Declare static LED data
static struct BurnLedInfo s_BurnLedInfo[8] = {
	{"LED 1", 0, 0, 0, 0, 0, 0, 0, 0x01, 0},
	{"LED 2", 0, 0, 0, 0, 0, 0, 0, 0x02, 0},
	{"LED 3", 0, 0, 0, 0, 0, 0, 0, 0x04, 0},
	{"LED 4", 0, 0, 0, 0, 0, 0, 0, 0x08, 0},
	{"LED 5", 0, 0, 0, 0, 0, 0, 0, 0x10, 0},
	{"LED 6", 0, 0, 0, 0, 0, 0, 0, 0x20, 0},
	{"LED 7", 0, 0, 0, 0, 0, 0, 0, 0x40, 0},
	{"LED 8", 0, 0, 0, 0, 0, 0, 0, 0x80, 0}
};

static struct BurnLedBlendInfo s_BurnLedBlendInfo = { 0, 0, 0, 0, 0, NULL, 0, 0, 0, 0 };

static INT32 s_BurnLedWidth = 4;
static INT32 s_BurnLedHeight = 4;

static UINT8 s_BurnLedStatus;

static INT32 s_BurnLedSizeDiv;
static UINT32 s_BurnLedColor;
static INT32 s_BurnLedSplitArea;
static INT32 s_BurnLedLayerfx;

// Simple implementation of the LED functions

void BurnLedReset()
{
	s_BurnLedStatus = 0;
	
	if (s_BurnLedBlendInfo.surface) {
		UINT32 *surface32 = (UINT32*)s_BurnLedBlendInfo.surface;
		
		for (INT32 i = 0; i < s_BurnLedBlendInfo.pitch * s_BurnLedBlendInfo.height; i++) {
			surface32[i] = 0;
		}
	}
}

INT32 BurnLedInit(INT32 num, INT32 xpos, INT32 ypos, INT32 xspace, INT32 yspace, INT32 splitarea, INT32 Layerfx)
{
	// Make sure that the burn LED system is not initialised twice 
	if (Debug_BurnLedInitted) {
		return 1;
	}
	
	s_BurnLedSizeDiv = 1;
	s_BurnLedColor = 0;
	s_BurnLedStatus = 0;
	s_BurnLedSplitArea = splitarea;
	s_BurnLedLayerfx = Layerfx;
	
	memset(&s_BurnLedBlendInfo, 0, sizeof(s_BurnLedBlendInfo));
	
	if (num > 8) num = 8;
	if (num < 0) num = 0;
	
	for (INT32 i = 0; i < 8; i++) {
		s_BurnLedInfo[i].status = 0;
		s_BurnLedInfo[i].xpos = xpos;
		s_BurnLedInfo[i].ypos = ypos;
		s_BurnLedInfo[i].xspa = xspace;
		s_BurnLedInfo[i].yspa = yspace;
	}
	
	Debug_BurnLedInitted = 1;
	
	return 0;
}

void BurnLedExit()
{
	Debug_BurnLedInitted = 0;
}

void BurnLedSetStatus(UINT8 led, UINT8 status)
{
	// Make sure that the burn LED system is initialised 
	if (!Debug_BurnLedInitted) {
		return;
	}
	
	if (status) {
		s_BurnLedStatus |= 1 << led;
	} else {
		s_BurnLedStatus &= ~(1 << led);
	}
}

// Render function - adapted for Metal build
void BurnLedRender(UINT32 color)
{
	// Make sure that the burn LED system is initialised 
	if (!Debug_BurnLedInitted) {
		return;
	}
	
	// Using external nScreenWidth/nScreenHeight instead of redefining them
	extern INT32 nScreenWidth, nScreenHeight;
	
	if (nScreenWidth <= 0 || nScreenHeight <= 0) {
		return;
	}
	
	s_BurnLedColor = color;
	
	// Implementation for Metal rendering would go here
	// For now we just print the LED status
	printf("BurnLedRender: LED Status 0x%02X\n", s_BurnLedStatus);
}

// Scan function needed for state saving
INT32 BurnLedScan(INT32 nAction, INT32 *pnMin)
{
	struct BurnArea ba;
	
	if (nAction & ACB_DRIVER_DATA) {
		if (pnMin && *pnMin < 0x029708) {
			*pnMin = 0x029708;
		}
		
		// Scan the LED status
		memset(&ba, 0, sizeof(ba));
		ba.Data = &s_BurnLedStatus;
		ba.nLen = sizeof(s_BurnLedStatus);
		ba.szName = "LED Status";
		BurnAcb(&ba);
	}
	
	return 0;
}
