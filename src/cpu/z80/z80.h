#ifndef _Z80_H_
#define _Z80_H_

#ifdef __cplusplus
extern "C" {
#endif

// Define __fastcall as empty for platforms that don't support it
#if defined(__APPLE__) || defined(__aarch64__) || defined(__arm64__)
#undef __fastcall
#define __fastcall
#endif

#define	CPUINFO_PTR_CPU_SPECIFIC	0x18000
#define Z80_CLEAR_LINE		0
#define Z80_ASSERT_LINE		1
#define Z80_INPUT_LINE_NMI	32

#include "z80daisy.h"

typedef union
{
#ifdef LSB_FIRST
	struct { UINT8 l,h,h2,h3; } b;
	struct { UINT16 l,h; } w;
#else
	struct { UINT8 h3,h2,h,l; } b;
	struct { UINT16 h,l; } w;
#endif
	UINT32 d;
} Z80_PAIR;

typedef struct
{
	Z80_PAIR	prvpc,pc,sp,af,bc,de,hl,ix,iy;
	Z80_PAIR	af2,bc2,de2,hl2,wz;
	UINT8	r,r2,iff1,iff2,halt,im,i;
	UINT8	nmi_state;			/* nmi line state */
	UINT8	nmi_pending;		/* nmi pending */
	UINT8	irq_state;			/* irq line state */
	UINT8   vector;             /* vector */
	UINT8	after_ei;			/* are we in the EI shadow? */
	UINT8	after_retn;			/* are we in the RETN shadow? */
	INT32   cycles_left;
	INT32   ICount;
	INT32   end_run;
	UINT32  EA;
	INT32   hold_irq;

	const struct z80_irq_daisy_chain *daisy;
	int (*irq_callback)(int irqline);

	int (*spectrum_tape_cb)();
	int spectrum_mode;
} Z80_Regs;

enum {
	Z80_PC=1, Z80_SP,
	Z80_A, Z80_B, Z80_C, Z80_D, Z80_E, Z80_H, Z80_L,
	Z80_AF, Z80_BC, Z80_DE, Z80_HL,
	Z80_IX, Z80_IY,	Z80_AF2, Z80_BC2, Z80_DE2, Z80_HL2,
	Z80_R, Z80_I, Z80_IM, Z80_IFF1, Z80_IFF2, Z80_HALT,
	Z80_DC0, Z80_DC1, Z80_DC2, Z80_DC3
};

enum {
	Z80_TABLE_op,
	Z80_TABLE_cb,
	Z80_TABLE_ed,
	Z80_TABLE_xy,
	Z80_TABLE_xycb,
	Z80_TABLE_ex	/* cycles counts for taken jr/jp/call and interrupt latency (rst opcodes) */
};

enum
{
	CPUINFO_PTR_Z80_CYCLE_TABLE = CPUINFO_PTR_CPU_SPECIFIC,
	CPUINFO_PTR_Z80_CYCLE_TABLE_LAST = CPUINFO_PTR_Z80_CYCLE_TABLE + Z80_TABLE_ex
};

extern void Z80Init();
extern void Z80InitContention(int is_on_type, void (*rastercallback)(int));
extern void Z80Contention_set_bank(int bankno);
extern void Z80Reset();
extern void Z80Exit();
extern int Z80Execute(int cycles);
extern void Z80Burn(int cycles);
extern void Z80SetIRQLine(int irqline, int state);
extern void Z80GetContext(void *pcontext);
extern void Z80SetContext(void *pcontext);
extern int Z80GetPC();
extern void Z80Scan(int nAction);
extern unsigned char Z80ReadByte(unsigned short address);
extern void Z80WriteByte(unsigned short address, unsigned char data);
extern unsigned char Z80ReadIO(unsigned short port);
extern void Z80WriteIO(unsigned short port, unsigned char data);
extern unsigned char Z80ReadCheat(unsigned short address);
extern void Z80WriteCheat(unsigned short address, unsigned char data);
extern void Z80SetIOReadHandler(unsigned char (*pread)(unsigned short));
extern void Z80SetIOWriteHandler(void (*pwrite)(unsigned short, unsigned char));
extern void Z80SetProgramReadHandler(unsigned char (*pread)(unsigned short));
extern void Z80SetProgramWriteHandler(void (*pwrite)(unsigned short, unsigned char));
extern void Z80SetCPUOpReadHandler(unsigned char (*pread)(unsigned short));
extern void Z80SetCPUOpArgReadHandler(unsigned char (*pread)(unsigned short));

extern int nZ80ICount, nZ80Cycles;
extern unsigned char **Z80CPUContext;

#ifdef __cplusplus
}
#endif

#endif /* _Z80_H_ */

