#ifndef _6309OPS_INCLUDE_H
#define _6309OPS_INCLUDE_H

#include "burnint.h"
#include "hd6309.h"

// We need to include definitions from hd6309.cpp because 6309ops.c uses them
extern hd6309_Regs hd6309;

/* flag bits in the cc register */
#define CC_C    0x01        /* Carry */
#define CC_V    0x02        /* Overflow */
#define CC_Z    0x04        /* Zero */
#define CC_N    0x08        /* Negative */
#define CC_II   0x10        /* Inhibit IRQ */
#define CC_H    0x20        /* Half (auxiliary) carry */
#define CC_IF   0x40        /* Inhibit FIRQ */
#define CC_E    0x80        /* entire state pushed */

/* flag bits in the md register */
#define MD_EM   0x01        /* Execution mode */
#define MD_FM   0x02        /* FIRQ mode */
#define MD_II   0x40        /* Illegal instruction */
#define MD_DZ   0x80        /* Division by zero */

// From hd6309.cpp
#define pPPC    hd6309.ppc
#define pPC     hd6309.pc
#define pU      hd6309.u
#define pS      hd6309.s
#define pX      hd6309.x
#define pY      hd6309.y
#define pV      hd6309.v
#define pD      hd6309.d
#define pW      hd6309.w

#define PPC     hd6309.ppc.w.l
#define PC      hd6309.pc.w.l
#define PCD     hd6309.pc.d
#define U       hd6309.u.w.l
#define UD      hd6309.u.d
#define S       hd6309.s.w.l
#define SD      hd6309.s.d
#define X       hd6309.x.w.l
#define XD      hd6309.x.d
#define Y       hd6309.y.w.l
#define YD      hd6309.y.d
#define V       hd6309.v.w.l
#define VD      hd6309.v.d
#define D       hd6309.d.w.l
#define A       hd6309.d.b.h
#define B       hd6309.d.b.l
#define W       hd6309.w.w.l
#define E       hd6309.w.b.h
#define F       hd6309.w.b.l
#define DP      hd6309.dp.b.h
#define DPD     hd6309.dp.d
#define CC      hd6309.cc
#define MD      hd6309.md

#define ea      hd6309.ea
#define EA      ea.w.l
#define EAD     ea.d

#define HD6309_INLINE static

// Macros from hd6309.cpp
#define CHANGE_PC change_pc(PCD)

#define HD6309_CWAI     8   /* set when CWAI is waiting for an interrupt */
#define HD6309_SYNC     16  /* set when SYNC is waiting for an interrupt */
#define HD6309_LDS      32  /* set when LDS occured at least once */

/* these are re-defined in hd6309.h TO RAM, ROM or functions in cpuintrf.c */
#define RM(mAddr)       HD6309_RDMEM(mAddr)
#define WM(mAddr,Value) HD6309_WRMEM(mAddr,Value)
#define ROP(mAddr)      HD6309_RDOP(mAddr)
#define ROP_ARG(mAddr)  HD6309_RDOP_ARG(mAddr)

/* macros to access memory */
#define IMMBYTE(b)  b = ROP_ARG(PCD); PC++
#define IMMWORD(w)  w.d = (ROP_ARG(PCD)<<8) | ROP_ARG((PCD+1)&0xffff); PC+=2
#define IMMLONG(w)  w.d = (ROP_ARG(PCD)<<24) + (ROP_ARG(PCD+1)<<16) + (ROP_ARG(PCD+2)<<8) + (ROP_ARG(PCD+3)); PC+=4

#define PUSHBYTE(b) --S; WM(SD,b)
#define PUSHWORD(w) --S; WM(SD,w.b.l); --S; WM(SD,w.b.h)
#define PULLBYTE(b) b = RM(SD); S++
#define PULLWORD(w) w = RM(SD)<<8; S++; w |= RM(SD); S++

#define PSHUBYTE(b) --U; WM(UD,b);
#define PSHUWORD(w) --U; WM(UD,w.b.l); --U; WM(UD,w.b.h)
#define PULUBYTE(b) b = RM(UD); U++
#define PULUWORD(w) w = RM(UD)<<8; U++; w |= RM(UD); U++

#define CLR_HNZVC   CC&=~(CC_H|CC_N|CC_Z|CC_V|CC_C)
#define CLR_NZV     CC&=~(CC_N|CC_Z|CC_V)
#define CLR_NZ      CC&=~(CC_N|CC_Z)
#define CLR_HNZC    CC&=~(CC_H|CC_N|CC_Z|CC_C)
#define CLR_NZVC    CC&=~(CC_N|CC_Z|CC_V|CC_C)
#define CLR_Z       CC&=~(CC_Z)
#define CLR_N       CC&=~(CC_N)
#define CLR_NZC     CC&=~(CC_N|CC_Z|CC_C)
#define CLR_ZC      CC&=~(CC_Z|CC_C)

/* macros for CC -- CC bits affected should be reset before calling */
#define SET_Z(a)        if(!a)SEZ
#define SET_Z8(a)       SET_Z((UINT8)a)
#define SET_Z16(a)      SET_Z((UINT16)a)
#define SET_N8(a)       CC|=((a&0x80)>>4)
#define SET_N16(a)      CC|=((a&0x8000)>>12)
#define SET_N32(a)      CC|=((a&0x8000)>>20)
#define SET_H(a,b,r)    CC|=(((a^b^r)&0x10)<<1)
#define SET_C8(a)       CC|=((a&0x100)>>8)
#define SET_C16(a)      CC|=((a&0x10000)>>16)
#define SET_V8(a,b,r)   CC|=(((a^b^r^(r>>1))&0x80)>>6)
#define SET_V16(a,b,r)  CC|=(((a^b^r^(r>>1))&0x8000)>>14)

#define SET_FLAGS8I(a)      {CC|=flags8i[(a)&0xff];}
#define SET_FLAGS8D(a)      {CC|=flags8d[(a)&0xff];}

/* combos */
#define SET_NZ8(a)          {SET_N8(a);SET_Z(a);}
#define SET_NZ16(a)         {SET_N16(a);SET_Z(a);}
#define SET_FLAGS8(a,b,r)   {SET_N8(r);SET_Z8(r);SET_V8(a,b,r);SET_C8(r);}
#define SET_FLAGS16(a,b,r)  {SET_N16(r);SET_Z16(r);SET_V16(a,b,r);SET_C16(r);}

#define NXORV               ((CC&CC_N)^((CC&CC_V)<<2))

/* for treating an unsigned byte as a signed word */
#define SIGNED(b) ((UINT16)(b&0x80?b|0xff00:b))
/* for treating an unsigned short as a signed long */
#define SIGNED_16(b) ((UINT32)(b&0x8000?b|0xffff0000:b))

/* macros for addressing modes (postbytes have their own code) */
#define DIRECT  EAD = DPD; IMMBYTE(ea.b.l)
#define IMM8    EAD = PCD; PC++
#define IMM16   EAD = PCD; PC+=2
#define EXTENDED IMMWORD(ea)

/* macros to set status flags */
#define SEC CC|=CC_C
#define CLC CC&=~CC_C
#define SEZ CC|=CC_Z
#define CLZ CC&=~CC_Z
#define SEN CC|=CC_N
#define CLN CC&=~CC_N
#define SEV CC|=CC_V
#define CLV CC&=~CC_V
#define SEH CC|=CC_H
#define CLH CC&=~CC_H

/* Macros to set mode flags */
#define SEDZ MD|=MD_DZ
#define CLDZ MD&=~MD_DZ
#define SEII MD|=MD_II
#define CLII MD&=~MD_II
#define SEFM MD|=MD_FM
#define CLFM MD&=~MD_FM
#define SEEM MD|=MD_EM
#define CLEM MD&=~MD_EM

/* macros for convenience */
#define DIRBYTE(b) {DIRECT;b=RM(EAD);}
#define DIRWORD(w) {DIRECT;w.d=RM16(EAD);}
#define DIRLONG(lng) {DIRECT;lng.w.h=RM16(EAD);lng.w.l=RM16(EAD+2);}
#define EXTBYTE(b) {EXTENDED;b=RM(EAD);}
#define EXTWORD(w) {EXTENDED;w.d=RM16(EAD);}
#define EXTLONG(lng) {EXTENDED;lng.w.h=RM16(EAD);lng.w.l=RM16(EAD+2);}

void fetch_effective_address();
void IIError();
void DZError();
void CHECK_IRQ_LINES();

// Forward declare functions implemented in 6309ops.c
void illegal();

#endif // _6309OPS_INCLUDE_H 