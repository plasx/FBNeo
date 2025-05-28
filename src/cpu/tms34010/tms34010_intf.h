#ifndef TMS34010_INTF_H
#define TMS34010_INTF_H

#include "burnint.h"

#ifdef __cplusplus
extern "C" {
#endif

// TMS34010 Interface
void TMS34010Init();
void TMS34010Reset();
void TMS34010Exit();
INT32 TMS34010Run(INT32 cycles);
void TMS34010SetIRQLine(INT32 line, INT32 state);
INT32 TMS34010GetActive();
INT32 TMS34010TotalCycles();
void TMS34010NewFrame();
INT32 TMS34010Scan(INT32 nAction);

// Memory handlers
void TMS34010MapMemory(UINT8 *mem, UINT32 start, UINT32 end, INT32 type);
void TMS34010SetReadHandler(UINT8 (*pHandler)(UINT32, UINT32));
void TMS34010SetWriteHandler(void (*pHandler)(UINT32, UINT32, UINT8));
UINT8 TMS34010ReadByte(UINT32 address);
void TMS34010WriteByte(UINT32 address, UINT8 value);
UINT16 TMS34010ReadWord(UINT32 address);
void TMS34010WriteWord(UINT32 address, UINT16 value);

// Additional control functions
void TMS34010SetTimerCallback(void (*callback)());
void TMS34010SetDisplayInfo(INT32 rowAddr, INT32 colAddr, INT32 vsblnk, INT32 veblnk, INT32 heblnk, INT32 hsblnk, INT32 htotal);

#ifdef __cplusplus
}
#endif

#endif // TMS34010_INTF_H 