/*
   i386 Disassembler

   Written by Ville Linde
   NEC V-Series support by Bryan McPhail (currently incomplete)
*/

#include "driver.h"

enum {
	PARAM_REG = 1,		/* 16 or 32-bit register */
	PARAM_REG8,			/* 8-bit register */
	PARAM_REG16,		/* 16-bit register */
	PARAM_REG2_32,		/* 32-bit register */
	PARAM_RM,			/* 16 or 32-bit memory or register */
	PARAM_RM8,			/* 8-bit memory or register */
	PARAM_RM16,			/* 16-bit memory or register */
	PARAM_I8,			/* 8-bit signed immediate */
	PARAM_I16,			/* 16-bit signed immediate */
	PARAM_UI8,			/* 8-bit unsigned immediate */
	PARAM_UI16,			/* 16-bit unsigned immediate */
	PARAM_IMM,			/* 16 or 32-bit immediate */
	PARAM_ADDR,			/* 16:16 or 16:32 address */
	PARAM_REL,			/* 16 or 32-bit PC-relative displacement */
	PARAM_REL8,			/* 8-bit PC-relative displacement */
	PARAM_MEM_OFFS_B,	/* 8-bit mem offset */
	PARAM_MEM_OFFS_V,	/* 16 or 32-bit mem offset */
	PARAM_SREG,			/* segment register */
	PARAM_CREG,			/* control register */
	PARAM_DREG,			/* debug register */
	PARAM_TREG,			/* test register */
	PARAM_1,			/* used by shift/rotate instructions */
	PARAM_AL,
	PARAM_CL,
	PARAM_DL,
	PARAM_BL,
	PARAM_AH,
	PARAM_CH,
	PARAM_DH,
	PARAM_BH,
	PARAM_DX,
	PARAM_EAX,			/* EAX or AX */
	PARAM_ECX,			/* ECX or CX */
	PARAM_EDX,			/* EDX or DX */
	PARAM_EBX,			/* EBX or BX */
	PARAM_ESP,			/* ESP or SP */
	PARAM_EBP,			/* EBP or BP */
	PARAM_ESI,			/* ESI or SI */
	PARAM_EDI,			/* EDI or DI */
};

enum {
	MODRM = 1,
	GROUP,
	FPU,
	VAR_NAME,
	OP_SIZE,
	ADDR_SIZE,
	TWO_BYTE,
	PREFIX,
	SEG_CS,
	SEG_DS,
	SEG_ES,
	SEG_FS,
	SEG_GS,
	SEG_SS
};

typedef struct {
	char mnemonic[32];
	UINT32 flags;
	UINT32 param1;
	UINT32 param2;
	UINT32 param3;
	offs_t dasm_flags;
} I386_OPCODE;

typedef struct {
	char mnemonic[32];
	const I386_OPCODE *opcode;
} GROUP_OP;

static UINT8 *opcode_ptr;
static const I386_OPCODE *opcode_table1 = 0;
static const I386_OPCODE *opcode_table2 = 0;

static const I386_OPCODE i386_opcode_table1[256] =
{
	// 0x00
	{"add",				MODRM,			PARAM_RM8,			PARAM_REG8,			0				},
	{"add",				MODRM,			PARAM_RM,			PARAM_REG,			0				},
	{"add",				MODRM,			PARAM_REG8,			PARAM_RM8,			0				},
	{"add",				MODRM,			PARAM_REG,			PARAM_RM,			0				},
	{"add",				0,				PARAM_AL,			PARAM_I8,			0				},
	{"add",				0,				PARAM_EAX,			PARAM_IMM,			0				},
	{"push    es",		0,				0,					0,					0				},
	{"pop     es",		0,				0,					0,					0				},
	{"or",				MODRM,			PARAM_RM8,			PARAM_REG8,			0				},
	{"or",				MODRM,			PARAM_RM,			PARAM_REG,			0				},
	{"or",				MODRM,			PARAM_REG8,			PARAM_RM8,			0				},
	{"or",				MODRM,			PARAM_REG,			PARAM_RM,			0				},
	{"or",				0,				PARAM_AL,			PARAM_I8,			0				},
	{"or",				0,				PARAM_EAX,			PARAM_IMM,			0				},
	{"push    cs",		0,				0,					0,					0				},
	{"two_byte",		TWO_BYTE,		0,					0,					0				},
	// 0x10
	{"adc",				MODRM,			PARAM_RM8,			PARAM_REG8,			0				},
	{"adc",				MODRM,			PARAM_RM,			PARAM_REG,			0				},
	{"adc",				MODRM,			PARAM_REG8,			PARAM_RM8,			0				},
	{"adc",				MODRM,			PARAM_REG,			PARAM_RM,			0				},
	{"adc",				0,				PARAM_AL,			PARAM_I8,			0				},
	{"adc",				0,				PARAM_EAX,			PARAM_IMM,			0				},
	{"push    ss",		0,				0,					0,					0				},
	{"pop     ss",		0,				0,					0,					0				},
	{"sbb",				MODRM,			PARAM_RM8,			PARAM_REG8,			0				},
	{"sbb",				MODRM,			PARAM_RM,			PARAM_REG,			0				},
	{"sbb",				MODRM,			PARAM_REG8,			PARAM_RM8,			0				},
	{"sbb",				MODRM,			PARAM_REG,			PARAM_RM,			0				},
	{"sbb",				0,				PARAM_AL,			PARAM_I8,			0				},
	{"sbb",				0,				PARAM_EAX,			PARAM_IMM,			0				},
	{"push    ds",		0,				0,					0,					0				},
	{"pop     ds",		0,				0,					0,					0				},
	// 0x20
	{"and",				MODRM,			PARAM_RM8,			PARAM_REG8,			0				},
	{"and",				MODRM,			PARAM_RM,			PARAM_REG,			0				},
	{"and",				MODRM,			PARAM_REG8,			PARAM_RM8,			0				},
	{"and",				MODRM,			PARAM_REG,			PARAM_RM,			0				},
	{"and",				0,				PARAM_AL,			PARAM_I8,			0				},
	{"and",				0,				PARAM_EAX,			PARAM_IMM,			0				},
	{"seg_es",			SEG_ES,			0,					0,					0				},
	{"daa",				0,				0,					0,					0				},
	{"sub",				MODRM,			PARAM_RM8,			PARAM_REG8,			0				},
	{"sub",				MODRM,			PARAM_RM,			PARAM_REG,			0				},
	{"sub",				MODRM,			PARAM_REG8,			PARAM_RM8,			0				},
	{"sub",				MODRM,			PARAM_REG,			PARAM_RM,			0				},
	{"sub",				0,				PARAM_AL,			PARAM_I8,			0				},
	{"sub",				0,				PARAM_EAX,			PARAM_IMM,			0				},
	{"seg_cs",			SEG_CS,			0,					0,					0				},
	{"das",				0,				0,					0,					0				},
	// 0x30
	{"xor",				MODRM,			PARAM_RM8,			PARAM_REG8,			0				},
	{"xor",				MODRM,			PARAM_RM,			PARAM_REG,			0				},
	{"xor",				MODRM,			PARAM_REG8,			PARAM_RM8,			0				},
	{"xor",				MODRM,			PARAM_REG,			PARAM_RM,			0				},
	{"xor",				0,				PARAM_AL,			PARAM_I8,			0				},
	{"xor",				0,				PARAM_EAX,			PARAM_IMM,			0				},
	{"seg_ss",			SEG_SS,			0,					0,					0				},
	{"aaa",				0,				0,					0,					0				},
	{"cmp",				MODRM,			PARAM_RM8,			PARAM_REG8,			0				},
	{"cmp",				MODRM,			PARAM_RM,			PARAM_REG,			0				},
	{"cmp",				MODRM,			PARAM_REG8,			PARAM_RM8,			0				},
	{"cmp",				MODRM,			PARAM_REG,			PARAM_RM,			0				},
	{"cmp",				0,				PARAM_AL,			PARAM_I8,			0				},
	{"cmp",				0,				PARAM_EAX,			PARAM_IMM,			0				},
	{"seg_ds",			SEG_DS,			0,					0,					0				},
	{"aas",				0,				0,					0,					0				},
	// 0x40
	{"inc",				0,				PARAM_EAX,			0,					0				},
	{"inc",				0,				PARAM_ECX,			0,					0				},
	{"inc",				0,				PARAM_EDX,			0,					0				},
	{"inc",				0,				PARAM_EBX,			0,					0				},
	{"inc",				0,				PARAM_ESP,			0,					0				},
	{"inc",				0,				PARAM_EBP,			0,					0				},
	{"inc",				0,				PARAM_ESI,			0,					0				},
	{"inc",				0,				PARAM_EDI,			0,					0				},
	{"dec",				0,				PARAM_EAX,			0,					0				},
	{"dec",				0,				PARAM_ECX,			0,					0				},
	{"dec",				0,				PARAM_EDX,			0,					0				},
	{"dec",				0,				PARAM_EBX,			0,					0				},
	{"dec",				0,				PARAM_ESP,			0,					0				},
	{"dec",				0,				PARAM_EBP,			0,					0				},
	{"dec",				0,				PARAM_ESI,			0,					0				},
	{"dec",				0,				PARAM_EDI,			0,					0				},
	// 0x50
	{"push",			0,				PARAM_EAX,			0,					0				},
	{"push",			0,				PARAM_ECX,			0,					0				},
	{"push",			0,				PARAM_EDX,			0,					0				},
	{"push",			0,				PARAM_EBX,			0,					0				},
	{"push",			0,				PARAM_ESP,			0,					0				},
	{"push",			0,				PARAM_EBP,			0,					0				},
	{"push",			0,				PARAM_ESI,			0,					0				},
	{"push",			0,				PARAM_EDI,			0,					0				},
	{"pop",				0,				PARAM_EAX,			0,					0				},
	{"pop",				0,				PARAM_ECX,			0,					0				},
	{"pop",				0,				PARAM_EDX,			0,					0				},
	{"pop",				0,				PARAM_EBX,			0,					0				},
	{"pop",				0,				PARAM_ESP,			0,					0				},
	{"pop",				0,				PARAM_EBP,			0,					0				},
	{"pop",				0,				PARAM_ESI,			0,					0				},
	{"pop",				0,				PARAM_EDI,			0,					0				},
	// 0x60
	{"pusha\0pushad",	VAR_NAME,		0,					0,					0				},
	{"popa\0popad",		VAR_NAME,		0,					0,					0				},
	{"bound",			MODRM,			PARAM_REG,			PARAM_RM,			0				},
	{"arpl",			MODRM,			PARAM_RM,			PARAM_REG16,		0				},
	{"seg_fs",			SEG_FS,			0,					0,					0				},
	{"seg_gs",			SEG_GS,			0,					0,					0				},
	{"op_size",			OP_SIZE,		0,					0,					0				},
	{"addr_size",		ADDR_SIZE,		0,					0,					0				},
	{"push",			0,				PARAM_IMM,			0,					0				},
	{"imul",			MODRM,			PARAM_REG,			PARAM_RM,			PARAM_IMM		},
	{"push",			0,				PARAM_I8,			0,					0				},
	{"imul",			MODRM,			PARAM_REG,			PARAM_RM,			PARAM_I8		},
	{"insb",			0,				0,					0,					0				},
	{"insw\0insd",		VAR_NAME,		0,					0,					0				},
	{"outsb",			0,				0,					0,					0				},
	{"outsw\0outsd",	VAR_NAME,		0,					0,					0				},
	// 0x70
	{"jo",				0,				PARAM_REL8,			0,					0				},
	{"jno",				0,				PARAM_REL8,			0,					0				},
	{"jb",				0,				PARAM_REL8,			0,					0				},
	{"jae",				0,				PARAM_REL8,			0,					0				},
	{"je",				0,				PARAM_REL8,			0,					0				},
	{"jne",				0,				PARAM_REL8,			0,					0				},
	{"jbe",				0,				PARAM_REL8,			0,					0				},
	{"ja",				0,				PARAM_REL8,			0,					0				},
	{"js",				0,				PARAM_REL8,			0,					0				},
	{"jns",				0,				PARAM_REL8,			0,					0				},
	{"jp",				0,				PARAM_REL8,			0,					0				},
	{"jnp",				0,				PARAM_REL8,			0,					0				},
	{"jl",				0,				PARAM_REL8,			0,					0				},
	{"jge",				0,				PARAM_REL8,			0,					0				},
	{"jle",				0,				PARAM_REL8,			0,					0				},
	{"jg",				0,				PARAM_REL8,			0,					0				},
	// 0x80
	{"group80",			GROUP,			0,					0,					0				},
	{"group81",			GROUP,			0,					0,					0				},
	{"group80",			GROUP,			0,					0,					0				},
	{"group83",			GROUP,			0,					0,					0				},
	{"test",			MODRM,			PARAM_RM8,			PARAM_REG8,			0				},
	{"test",			MODRM,			PARAM_RM,			PARAM_REG,			0				},
	{"xchg",			MODRM,			PARAM_REG8,			PARAM_RM8,			0				},
	{"xchg",			MODRM,			PARAM_REG,			PARAM_RM,			0				},
	{"mov",				MODRM,			PARAM_RM8,			PARAM_REG8,			0				},
	{"mov",				MODRM,			PARAM_RM,			PARAM_REG,			0				},
	{"mov",				MODRM,			PARAM_REG8,			PARAM_RM8,			0				},
	{"mov",				MODRM,			PARAM_REG,			PARAM_RM,			0				},
	{"mov",				MODRM,			PARAM_RM,			PARAM_SREG,			0				},
	{"lea",				MODRM,			PARAM_REG,			PARAM_RM,			0				},
	{"mov",				MODRM,			PARAM_SREG,			PARAM_RM,			0				},
	{"pop",				MODRM,			PARAM_RM,			0,					0				},
	// 0x90
	{"nop",				0,				0,					0,					0				},
	{"xchg",			0,				PARAM_EAX,			PARAM_ECX,			0				},
	{"xchg",			0,				PARAM_EAX,			PARAM_EDX,			0				},
	{"xchg",			0,				PARAM_EAX,			PARAM_EBX,			0				},
	{"xchg",			0,				PARAM_EAX,			PARAM_ESP,			0				},
	{"xchg",			0,				PARAM_EAX,			PARAM_EBP,			0				},
	{"xchg",			0,				PARAM_EAX,			PARAM_ESI,			0				},
	{"xchg",			0,				PARAM_EAX,			PARAM_EDI,			0				},
	{"cbw\0cwde",		VAR_NAME,		0,					0,					0				},
	{"cwd\0cdq",		VAR_NAME,		0,					0,					0				},
	{"call",			0,				PARAM_ADDR,			0,					0,				DASMFLAG_STEP_OVER},
	{"wait",			0,				0,					0,					0				},
	{"pushf\0pushfd",	VAR_NAME,		0,					0,					0				},
	{"popf\0popfd",		VAR_NAME,		0,					0,					0				},
	{"sahf",			0,				0,					0,					0				},
	{"lahf",			0,				0,					0,					0				},
	// 0xa0
	{"mov",				0,				PARAM_AL,			PARAM_MEM_OFFS_V,	0				},
	{"mov",				0,				PARAM_EAX,			PARAM_MEM_OFFS_V,	0				},
	{"mov",				0,				PARAM_MEM_OFFS_V,	PARAM_AL,			0				},
	{"mov",				0,				PARAM_MEM_OFFS_V,	PARAM_EAX,			0				},
	{"movsb",			0,				0,					0,					0				},
	{"movsw\0movsd",	VAR_NAME,		0,					0,					0				},
	{"cmpsb",			0,				0,					0,					0				},
	{"cmpsw\0cmpsd",	VAR_NAME,		0,					0,					0				},
	{"test",			0,				PARAM_AL,			PARAM_I8,			0				},
	{"test",			0,				PARAM_EAX,			PARAM_IMM,			0				},
	{"stosb",			0,				0,					0,					0				},
	{"stosw\0stosd",	VAR_NAME,		0,					0,					0				},
	{"lodsb",			0,				0,					0,					0				},
	{"lodsw\0lodsd",	VAR_NAME,		0,					0,					0				},
	{"scasb",			0,				0,					0,					0				},
	{"scasw\0scasd",	VAR_NAME,		0,					0,					0				},
	// 0xb0
	{"mov",				0,				PARAM_AL,			PARAM_I8,			0				},
	{"mov",				0,				PARAM_CL,			PARAM_I8,			0				},
	{"mov",				0,				PARAM_DL,			PARAM_I8,			0				},
	{"mov",				0,				PARAM_BL,			PARAM_I8,			0				},
	{"mov",				0,				PARAM_AH,			PARAM_I8,			0				},
	{"mov",				0,				PARAM_CH,			PARAM_I8,			0				},
	{"mov",				0,				PARAM_DH,			PARAM_I8,			0				},
	{"mov",				0,				PARAM_BH,			PARAM_I8,			0				},
	{"mov",				0,				PARAM_EAX,			PARAM_IMM,			0				},
	{"mov",				0,				PARAM_ECX,			PARAM_IMM,			0				},
	{"mov",				0,				PARAM_EDX,			PARAM_IMM,			0				},
	{"mov",				0,				PARAM_EBX,			PARAM_IMM,			0				},
	{"mov",				0,				PARAM_ESP,			PARAM_IMM,			0				},
	{"mov",				0,				PARAM_EBP,			PARAM_IMM,			0				},
	{"mov",				0,				PARAM_ESI,			PARAM_IMM,			0				},
	{"mov",				0,				PARAM_EDI,			PARAM_IMM,			0				},
	// 0xc0
	{"groupC0",			GROUP,			0,					0,					0				},
	{"groupC1",			GROUP,			0,					0,					0				},
	{"ret",				0,				PARAM_I16,			0,					0,				DASMFLAG_STEP_OUT},
	{"ret",				0,				0,					0,					0,				DASMFLAG_STEP_OUT},
	{"les",				MODRM,			PARAM_REG,			PARAM_RM,			0				},
	{"lds",				MODRM,			PARAM_REG,			PARAM_RM,			0				},
	{"mov",				MODRM,			PARAM_RM8,			PARAM_I8,			0				},
	{"mov",				MODRM,			PARAM_RM,			PARAM_IMM,			0				},
	{"enter",			0,				PARAM_I16,			PARAM_I8,			0				},
	{"leave",			0,				0,					0,					0				},
	{"retf",			0,				PARAM_I16,			0,					0,				DASMFLAG_STEP_OUT},
	{"retf",			0,				0,					0,					0,				DASMFLAG_STEP_OUT},
	{"int 3",			0,				0,					0,					0,				DASMFLAG_STEP_OVER},
	{"int",				0,				PARAM_UI8,			0,					0,				DASMFLAG_STEP_OVER},
	{"into",			0,				0,					0,					0				},
	{"iret",			0,				0,					0,					0,				DASMFLAG_STEP_OUT},
	// 0xd0
	{"groupD0",			GROUP,			0,					0,					0				},
	{"groupD1",			GROUP,			0,					0,					0				},
	{"groupD2",			GROUP,			0,					0,					0				},
	{"groupD3",			GROUP,			0,					0,					0				},
	{"aam",				0,				PARAM_I8,			0,					0				},
	{"aad",				0,				PARAM_I8,			0,					0				},
	{"???",				0,				0,					0,					0				},
	{"xlat",			0,				0,					0,					0				},
	{"escape",			FPU,			0,					0,					0				},
	{"escape",			FPU,			0,					0,					0				},
	{"escape",			FPU,			0,					0,					0				},
	{"escape",			FPU,			0,					0,					0				},
	{"escape",			FPU,			0,					0,					0				},
	{"escape",			FPU,			0,					0,					0				},
	{"escape",			FPU,			0,					0,					0				},
	{"escape",			FPU,			0,					0,					0				},
	// 0xe0
	{"loopne",			0,				PARAM_REL8,			0,					0,				DASMFLAG_STEP_OVER},
	{"loopz",			0,				PARAM_REL8,			0,					0,				DASMFLAG_STEP_OVER},
	{"loop",			0,				PARAM_REL8,			0,					0,				DASMFLAG_STEP_OVER},
	{"jcxz\0jecxz",		VAR_NAME,		PARAM_REL8,			0,					0				},
	{"in",				0,				PARAM_AL,			PARAM_UI8,			0				},
	{"in",				0,				PARAM_EAX,			PARAM_UI8,			0				},
	{"out",				0,				PARAM_AL,			PARAM_UI8,			0				},
	{"out",				0,				PARAM_EAX,			PARAM_UI8,			0				},
	{"call",			0,				PARAM_REL,			0,					0,				DASMFLAG_STEP_OVER},
	{"jmp",				0,				PARAM_REL,			0,					0				},
	{"jmp",				0,				PARAM_ADDR,			0,					0				},
	{"jmp",				0,				PARAM_REL8,			0,					0				},
	{"in",				0,				PARAM_AL,			PARAM_DX,			0				},
	{"in",				0,				PARAM_EAX,			PARAM_DX,			0				},
	{"out",				0,				PARAM_AL,			PARAM_DX,			0				},
	{"out",				0,				PARAM_EAX,			PARAM_DX,			0				},
	// 0xf0
	{"lock",			0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"repne",			PREFIX,			0,					0,					0				},
	{"rep",				PREFIX,			0,					0,					0				},
	{"hlt",				0,				0,					0,					0				},
	{"cmc",				0,				0,					0,					0				},
	{"groupF6",			GROUP,			0,					0,					0				},
	{"groupF7",			GROUP,			0,					0,					0				},
	{"clc",				0,				0,					0,					0				},
	{"stc",				0,				0,					0,					0				},
	{"cli",				0,				0,					0,					0				},
	{"sti",				0,				0,					0,					0				},
	{"cld",				0,				0,					0,					0				},
	{"std",				0,				0,					0,					0				},
	{"groupFE",			GROUP,			0,					0,					0				},
	{"groupFF",			GROUP,			0,					0,					0				}
};

static const I386_OPCODE i386_opcode_table2[256] =
{
	// 0x00
	{"group0F00",		GROUP,			0,					0,					0				},
	{"group0F01",		GROUP,			0,					0,					0				},
	{"lar",				MODRM,			PARAM_REG,			PARAM_RM,			0				},
	{"lsl",				MODRM,			PARAM_REG,			PARAM_RM,			0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"clts",			0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"invd",			0,				0,					0,					0				},
	{"wbinvd",			0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"ud2",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	// 0x10
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	// 0x20
	{"mov",				MODRM,			PARAM_REG2_32,		PARAM_CREG,			0				},
	{"mov",				MODRM,			PARAM_REG2_32,		PARAM_DREG,			0				},
	{"mov",				MODRM,			PARAM_CREG,			PARAM_REG2_32,		0				},
	{"mov",				MODRM,			PARAM_DREG,			PARAM_REG2_32,		0				},
	{"mov",				MODRM,			PARAM_REG2_32,		PARAM_TREG,			0				},
	{"???",				0,				0,					0,					0				},
	{"mov",				MODRM,			PARAM_TREG,			PARAM_REG2_32,		0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	// 0x30
	{"wrmsr",			0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"rdmsr",			0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	// 0x40
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	// 0x50
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	// 0x60
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	// 0x70
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	// 0x80
	{"jo",				0,				PARAM_REL,			0,					0				},
	{"jno",				0,				PARAM_REL,			0,					0				},
	{"jb",				0,				PARAM_REL,			0,					0				},
	{"jae",				0,				PARAM_REL,			0,					0				},
	{"je",				0,				PARAM_REL,			0,					0				},
	{"jne",				0,				PARAM_REL,			0,					0				},
	{"jbe",				0,				PARAM_REL,			0,					0				},
	{"ja",				0,				PARAM_REL,			0,					0				},
	{"js",				0,				PARAM_REL,			0,					0				},
	{"jns",				0,				PARAM_REL,			0,					0				},
	{"jp",				0,				PARAM_REL,			0,					0				},
	{"jnp",				0,				PARAM_REL,			0,					0				},
	{"jl",				0,				PARAM_REL,			0,					0				},
	{"jge",				0,				PARAM_REL,			0,					0				},
	{"jle",				0,				PARAM_REL,			0,					0				},
	{"jg",				0,				PARAM_REL,			0,					0				},
	// 0x90
	{"seto",			MODRM,			PARAM_RM8,			0,					0				},
	{"setno",			MODRM,			PARAM_RM8,			0,					0				},
	{"setb",			MODRM,			PARAM_RM8,			0,					0				},
	{"setae",			MODRM,			PARAM_RM8,			0,					0				},
	{"sete",			MODRM,			PARAM_RM8,			0,					0				},
	{"setne",			MODRM,			PARAM_RM8,			0,					0				},
	{"setbe",			MODRM,			PARAM_RM8,			0,					0				},
	{"seta",			MODRM,			PARAM_RM8,			0,					0				},
	{"sets",			MODRM,			PARAM_RM8,			0,					0				},
	{"setns",			MODRM,			PARAM_RM8,			0,					0				},
	{"setp",			MODRM,			PARAM_RM8,			0,					0				},
	{"setnp",			MODRM,			PARAM_RM8,			0,					0				},
	{"setl",			MODRM,			PARAM_RM8,			0,					0				},
	{"setge",			MODRM,			PARAM_RM8,			0,					0				},
	{"setle",			MODRM,			PARAM_RM8,			0,					0				},
	{"setg",			MODRM,			PARAM_RM8,			0,					0				},
	// 0xa0
	{"push    fs",		0,				0,					0,					0				},
	{"pop     fs",		0,				0,					0,					0				},
	{"cpuid",			0,				0,					0,					0				},
	{"bt",				MODRM,			PARAM_RM,			PARAM_REG,			0				},
	{"shld",			MODRM,			PARAM_RM,			PARAM_REG,			PARAM_I8		},
	{"shld",			MODRM,			PARAM_RM,			PARAM_REG,			PARAM_CL		},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"push    gs",		0,				0,					0,					0				},
	{"pop     gs",		0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"bts",				MODRM,			PARAM_RM,			PARAM_REG,			0				},
	{"shrd",			MODRM,			PARAM_RM,			PARAM_REG,			PARAM_I8		},
	{"shrd",			MODRM,			PARAM_RM,			PARAM_REG,			PARAM_CL		},
	{"???",				0,				0,					0,					0				},
	{"imul",			MODRM,			PARAM_REG,			PARAM_RM,			0				},
	// 0xb0
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"lss",				MODRM,			PARAM_REG,			PARAM_RM,			0				},
	{"btr",				MODRM,			PARAM_RM,			PARAM_REG,			0				},
	{"lfs",				MODRM,			PARAM_REG,			PARAM_RM,			0				},
	{"lgs",				MODRM,			PARAM_REG,			PARAM_RM,			0				},
	{"movzx",			MODRM,			PARAM_REG,			PARAM_RM8,			0				},
	{"movzx",			MODRM,			PARAM_REG,			PARAM_RM16,			0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"group0FBA",		GROUP,			0,					0,					0				},
	{"btc",				MODRM,			PARAM_RM,			PARAM_REG,			0				},
	{"bsf",				MODRM,			PARAM_REG,			PARAM_RM,			0				},
	{"bsr",				MODRM,			PARAM_REG,			PARAM_RM,			0,				DASMFLAG_STEP_OVER},
	{"movsx",			MODRM,			PARAM_REG,			PARAM_RM8,			0				},
	{"movsx",			MODRM,			PARAM_REG,			PARAM_RM16,			0				},
	// 0xc0
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	// 0xd0
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	// 0xe0
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	// 0xf0
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				}
};

static const I386_OPCODE necv_opcode_table2[256] =
{
	// 0x00
	{"group0F00",		GROUP,			0,					0,					0				},
	{"group0F01",		GROUP,			0,					0,					0				},
	{"lar",				MODRM,			PARAM_REG,			PARAM_RM,			0				},
	{"lsl",				MODRM,			PARAM_REG,			PARAM_RM,			0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"clts",			0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"ud2",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	// 0x10 - NEC V series only
	{"test1",			0,				PARAM_RM8,			PARAM_1,			0				},
	{"test1",			0,				PARAM_RM16,			PARAM_1,			0				},
	{"clr1",			0,				PARAM_RM8,			PARAM_1,			0				},
	{"clr1",			0,				PARAM_RM16,			PARAM_1,			0				},
	{"set1",			0,				PARAM_RM8,			PARAM_1,			0				},
	{"set1",			0,				PARAM_RM16,			PARAM_1,			0				},
	{"not1",			0,				PARAM_RM8,			PARAM_1,			0				},
	{"not1",			0,				PARAM_RM16,			PARAM_1,			0				},
	{"test1",			0,				PARAM_RM8,			PARAM_I8,			0				},
	{"test1",			0,				PARAM_RM16,			PARAM_I8,			0				},
	{"clr1",			0,				PARAM_RM8,			PARAM_I8,			0				},
	{"clr1",			0,				PARAM_RM16,			PARAM_I8,			0				},
	{"set1",			0,				PARAM_RM8,			PARAM_I8,			0				},
	{"set1",			0,				PARAM_RM16,			PARAM_I8,			0				},
	{"not1",			0,				PARAM_RM8,			PARAM_I8,			0				},
	{"not1",			0,				PARAM_RM16,			PARAM_I8,			0				},
	// 0x20
	{"mov",				MODRM,			PARAM_REG,			PARAM_CREG,			0				},
	{"mov",				MODRM,			PARAM_REG,			PARAM_DREG,			0				},
	{"mov",				MODRM,			PARAM_CREG,			PARAM_REG,			0				},
	{"mov",				MODRM,			PARAM_DREG,			PARAM_REG,			0				},
	{"mov",				MODRM,			PARAM_REG,			PARAM_TREG,			0				},
	{"???",				0,				0,					0,					0				},
	{"mov",				MODRM,			PARAM_TREG,			PARAM_REG,			0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	// 0x30
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	// 0x40
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	// 0x50
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	// 0x60
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	// 0x70
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	// 0x80
	{"jo",				0,				PARAM_REL,			0,					0				},
	{"jno",				0,				PARAM_REL,			0,					0				},
	{"jb",				0,				PARAM_REL,			0,					0				},
	{"jae",				0,				PARAM_REL,			0,					0				},
	{"je",				0,				PARAM_REL,			0,					0				},
	{"jne",				0,				PARAM_REL,			0,					0				},
	{"jbe",				0,				PARAM_REL,			0,					0				},
	{"ja",				0,				PARAM_REL,			0,					0				},
	{"js",				0,				PARAM_REL,			0,					0				},
	{"jns",				0,				PARAM_REL,			0,					0				},
	{"jp",				0,				PARAM_REL,			0,					0				},
	{"jnp",				0,				PARAM_REL,			0,					0				},
	{"jl",				0,				PARAM_REL,			0,					0				},
	{"jge",				0,				PARAM_REL,			0,					0				},
	{"jle",				0,				PARAM_REL,			0,					0				},
	{"jg",				0,				PARAM_REL,			0,					0				},
	// 0x90
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"fint",			0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	// 0xa0
	{"push   fs",		0,				0,					0,					0				},
	{"pop    fs",		0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"bt",				MODRM,			PARAM_RM,			PARAM_REG,			0				},
	{"shld",			MODRM,			PARAM_RM,			PARAM_REG,			PARAM_I8		},
	{"shld",			MODRM,			PARAM_RM,			PARAM_REG,			PARAM_CL		},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"push   gs",		0,				0,					0,					0				},
	{"pop    gs",		0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"bts",				MODRM,			PARAM_RM,			PARAM_REG,			0				},
	{"shrd",			MODRM,			PARAM_RM,			PARAM_REG,			PARAM_I8		},
	{"shrd",			MODRM,			PARAM_RM,			PARAM_REG,			PARAM_CL		},
	{"???",				0,				0,					0,					0				},
	{"imul",			MODRM,			PARAM_REG,			PARAM_RM,			0				},
	// 0xb0
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"lss",				MODRM,			PARAM_REG,			PARAM_RM,			0				},
	{"btr",				MODRM,			PARAM_RM,			PARAM_REG,			0				},
	{"lfs",				MODRM,			PARAM_REG,			PARAM_RM,			0				},
	{"lgs",				MODRM,			PARAM_REG,			PARAM_RM,			0				},
	{"movzx",			MODRM,			PARAM_REG,			PARAM_RM8,			0				},
	{"movzx",			MODRM,			PARAM_REG,			PARAM_RM16,			0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"group0FBA",		GROUP,			0,					0,					0				},
	{"btc",				MODRM,			PARAM_RM,			PARAM_REG,			0				},
	{"bsf",				MODRM,			PARAM_REG,			PARAM_RM,			0				},
	{"bsr",				MODRM,			PARAM_REG,			PARAM_RM,			0,				DASMFLAG_STEP_OVER},
	{"movsx",			MODRM,			PARAM_REG,			PARAM_RM8,			0				},
	{"movsx",			MODRM,			PARAM_REG,			PARAM_RM16,			0				},
	// 0xc0
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	// 0xd0
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	// 0xe0
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	// 0xf0
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				}
};

static const I386_OPCODE group80_table[8] =
{
	{"add",				0,				PARAM_RM8,			PARAM_I8,			0				},
	{"or",				0,				PARAM_RM8,			PARAM_I8,			0				},
	{"adc",				0,				PARAM_RM8,			PARAM_I8,			0				},
	{"sbb",				0,				PARAM_RM8,			PARAM_I8,			0				},
	{"and",				0,				PARAM_RM8,			PARAM_I8,			0				},
	{"sub",				0,				PARAM_RM8,			PARAM_I8,			0				},
	{"xor",				0,				PARAM_RM8,			PARAM_I8,			0				},
	{"cmp",				0,				PARAM_RM8,			PARAM_I8,			0				}
};

static const I386_OPCODE group81_table[8] =
{
	{"add",				0,				PARAM_RM,			PARAM_IMM,			0				},
	{"or",				0,				PARAM_RM,			PARAM_IMM,			0				},
	{"adc",				0,				PARAM_RM,			PARAM_IMM,			0				},
	{"sbb",				0,				PARAM_RM,			PARAM_IMM,			0				},
	{"and",				0,				PARAM_RM,			PARAM_IMM,			0				},
	{"sub",				0,				PARAM_RM,			PARAM_IMM,			0				},
	{"xor",				0,				PARAM_RM,			PARAM_IMM,			0				},
	{"cmp",				0,				PARAM_RM,			PARAM_IMM,			0				}
};

static const I386_OPCODE group83_table[8] =
{
	{"add",				0,				PARAM_RM,			PARAM_I8,			0				},
	{"or",				0,				PARAM_RM,			PARAM_I8,			0				},
	{"adc",				0,				PARAM_RM,			PARAM_I8,			0				},
	{"sbb",				0,				PARAM_RM,			PARAM_I8,			0				},
	{"and",				0,				PARAM_RM,			PARAM_I8,			0				},
	{"sub",				0,				PARAM_RM,			PARAM_I8,			0				},
	{"xor",				0,				PARAM_RM,			PARAM_I8,			0				},
	{"cmp",				0,				PARAM_RM,			PARAM_I8,			0				}
};

static const I386_OPCODE groupC0_table[8] =
{
	{"rol",				0,				PARAM_RM8,			PARAM_I8,			0				},
	{"ror",				0,				PARAM_RM8,			PARAM_I8,			0				},
	{"rcl",				0,				PARAM_RM8,			PARAM_I8,			0				},
	{"rcr",				0,				PARAM_RM8,			PARAM_I8,			0				},
	{"shl",				0,				PARAM_RM8,			PARAM_I8,			0				},
	{"shr",				0,				PARAM_RM8,			PARAM_I8,			0				},
	{"sal",				0,				PARAM_RM8,			PARAM_I8,			0				},
	{"sar",				0,				PARAM_RM8,			PARAM_I8,			0				}
};

static const I386_OPCODE groupC1_table[8] =
{
	{"rol",				0,				PARAM_RM,			PARAM_I8,			0				},
	{"ror",				0,				PARAM_RM,			PARAM_I8,			0				},
	{"rcl",				0,				PARAM_RM,			PARAM_I8,			0				},
	{"rcr",				0,				PARAM_RM,			PARAM_I8,			0				},
	{"shl",				0,				PARAM_RM,			PARAM_I8,			0				},
	{"shr",				0,				PARAM_RM,			PARAM_I8,			0				},
	{"sal",				0,				PARAM_RM,			PARAM_I8,			0				},
	{"sar",				0,				PARAM_RM,			PARAM_I8,			0				}
};

static const I386_OPCODE groupD0_table[8] =
{
	{"rol",				0,				PARAM_RM8,			PARAM_1,			0				},
	{"ror",				0,				PARAM_RM8,			PARAM_1,			0				},
	{"rcl",				0,				PARAM_RM8,			PARAM_1,			0				},
	{"rcr",				0,				PARAM_RM8,			PARAM_1,			0				},
	{"shl",				0,				PARAM_RM8,			PARAM_1,			0				},
	{"shr",				0,				PARAM_RM8,			PARAM_1,			0				},
	{"sal",				0,				PARAM_RM8,			PARAM_1,			0				},
	{"sar",				0,				PARAM_RM8,			PARAM_1,			0				}
};

static const I386_OPCODE groupD1_table[8] =
{
	{"rol",				0,				PARAM_RM,			PARAM_1,			0				},
	{"ror",				0,				PARAM_RM,			PARAM_1,			0				},
	{"rcl",				0,				PARAM_RM,			PARAM_1,			0				},
	{"rcr",				0,				PARAM_RM,			PARAM_1,			0				},
	{"shl",				0,				PARAM_RM,			PARAM_1,			0				},
	{"shr",				0,				PARAM_RM,			PARAM_1,			0				},
	{"sal",				0,				PARAM_RM,			PARAM_1,			0				},
	{"sar",				0,				PARAM_RM,			PARAM_1,			0				}
};

static const I386_OPCODE groupD2_table[8] =
{
	{"rol",				0,				PARAM_RM8,			PARAM_CL,			0				},
	{"ror",				0,				PARAM_RM8,			PARAM_CL,			0				},
	{"rcl",				0,				PARAM_RM8,			PARAM_CL,			0				},
	{"rcr",				0,				PARAM_RM8,			PARAM_CL,			0				},
	{"shl",				0,				PARAM_RM8,			PARAM_CL,			0				},
	{"shr",				0,				PARAM_RM8,			PARAM_CL,			0				},
	{"sal",				0,				PARAM_RM8,			PARAM_CL,			0				},
	{"sar",				0,				PARAM_RM8,			PARAM_CL,			0				}
};

static const I386_OPCODE groupD3_table[8] =
{
	{"rol",				0,				PARAM_RM,			PARAM_CL,			0				},
	{"ror",				0,				PARAM_RM,			PARAM_CL,			0				},
	{"rcl",				0,				PARAM_RM,			PARAM_CL,			0				},
	{"rcr",				0,				PARAM_RM,			PARAM_CL,			0				},
	{"shl",				0,				PARAM_RM,			PARAM_CL,			0				},
	{"shr",				0,				PARAM_RM,			PARAM_CL,			0				},
	{"sal",				0,				PARAM_RM,			PARAM_CL,			0				},
	{"sar",				0,				PARAM_RM,			PARAM_CL,			0				}
};

static const I386_OPCODE groupF6_table[8] =
{
	{"test",			0,				PARAM_RM8,			PARAM_I8,			0				},
	{"???",				0,				0,					0,					0				},
	{"not",				0,				PARAM_RM8,			0,					0				},
	{"neg",				0,				PARAM_RM8,			0,					0				},
	{"mul",				0,				PARAM_RM8,			0,					0				},
	{"imul",			0,				PARAM_RM8,			0,					0				},
	{"div",				0,				PARAM_RM8,			0,					0				},
	{"idiv",			0,				PARAM_RM8,			0,					0				}
};

static const I386_OPCODE groupF7_table[8] =
{
	{"test",			0,				PARAM_RM,			PARAM_IMM,			0				},
	{"???",				0,				0,					0,					0				},
	{"not",				0,				PARAM_RM,			0,					0				},
	{"neg",				0,				PARAM_RM,			0,					0				},
	{"mul",				0,				PARAM_RM,			0,					0				},
	{"imul",			0,				PARAM_RM,			0,					0				},
	{"div",				0,				PARAM_RM,			0,					0				},
	{"idiv",			0,				PARAM_RM,			0,					0				}
};

static const I386_OPCODE groupFE_table[8] =
{
	{"inc",				0,				PARAM_RM8,			0,					0				},
	{"dec",				0,				PARAM_RM8,			0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				}
};

static const I386_OPCODE groupFF_table[8] =
{
	{"inc",				0,				PARAM_RM,			0,					0				},
	{"dec",				0,				PARAM_RM,			0,					0				},
	{"call",			0,				PARAM_RM,			0,					0,				DASMFLAG_STEP_OVER},
	{"call",			0,				PARAM_RM,			0,					0,				DASMFLAG_STEP_OVER},
	{"jmp",				0,				PARAM_RM,			0,					0				},
	{"jmp",				0,				PARAM_RM,			0,					0				},
	{"push",			0,				PARAM_RM,			0,					0				},
	{"???",				0,				0,					0,					0				}
};

static const I386_OPCODE group0F00_table[8] =
{
	{"sldt",			0,				PARAM_RM,			0,					0				},
	{"str",				0,				PARAM_RM,			0,					0				},
	{"lldt",			0,				PARAM_RM,			0,					0				},
	{"ltr",				0,				PARAM_RM,			0,					0				},
	{"verr",			0,				PARAM_RM,			0,					0				},
	{"verw",			0,				PARAM_RM,			0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				}
};

static const I386_OPCODE group0F01_table[8] =
{
	{"sgdt",			0,				PARAM_RM,			0,					0				},
	{"sidt",			0,				PARAM_RM,			0,					0				},
	{"lgdt",			0,				PARAM_RM,			0,					0				},
	{"lidt",			0,				PARAM_RM,			0,					0				},
	{"smsw",			0,				PARAM_RM,			0,					0				},
	{"???",				0,				0,					0,					0				},
	{"lmsw",			0,				PARAM_RM,			0,					0				},
	{"???",				0,				0,					0,					0				}
};

static const I386_OPCODE group0FBA_table[8] =
{
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"???",				0,				0,					0,					0				},
	{"bt",				0,				PARAM_RM,			PARAM_I8,			0				},
	{"bts",				0,				PARAM_RM,			PARAM_I8,			0				},
	{"btr",				0,				PARAM_RM,			PARAM_I8,			0				},
	{"btc",				0,				PARAM_RM,			PARAM_I8,			0				}
};

static const GROUP_OP group_op_table[] =
{
	{ "group80",			group80_table			},
	{ "group81",			group81_table			},
	{ "group83",			group83_table			},
	{ "groupC0",			groupC0_table			},
	{ "groupC1",			groupC1_table			},
	{ "groupD0",			groupD0_table			},
	{ "groupD1",			groupD1_table			},
	{ "groupD2",			groupD2_table			},
	{ "groupD3",			groupD3_table			},
	{ "groupF6",			groupF6_table			},
	{ "groupF7",			groupF7_table			},
	{ "groupFE",			groupFE_table			},
	{ "groupFF",			groupFF_table			},
	{ "group0F00",			group0F00_table			},
	{ "group0F01",			group0F01_table			},
	{ "group0FBA",			group0FBA_table			}
};



static const char i386_reg[2][8][4] =
{
	{"ax",	"cx",	"dx",	"bx",	"sp",	"bp",	"si",	"di"},
	{"eax",	"ecx",	"edx",	"ebx",	"esp",	"ebp",	"esi",	"edi"}
};

static const char i386_reg8[8][4] = {"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"};
static const char i386_sreg[8][4] = {"es", "cs", "ss", "ds", "fs", "gs", "???", "???"};

static int address_size;
static int operand_size;
static UINT32 pc;
static UINT8 modrm;
static UINT32 segment;
static offs_t dasm_flags;
static char modrm_string[256];

#define MODRM_REG1	((modrm >> 3) & 0x7)
#define MODRM_REG2	(modrm & 0x7)

INLINE UINT8 FETCH(void)
{
	pc++;
	return *opcode_ptr++;
}

INLINE UINT16 FETCH16(void)
{
	UINT16 d;
	d = opcode_ptr[0] | (opcode_ptr[1] << 8);
	opcode_ptr += 2;
	pc += 2;
	return d;
}

INLINE UINT32 FETCH32(void)
{
	UINT32 d;
	d = opcode_ptr[0] | (opcode_ptr[1] << 8) | (opcode_ptr[2] << 16) | (opcode_ptr[3] << 24);
	opcode_ptr += 4;
	pc += 4;
	return d;
}

INLINE UINT8 FETCHD(void)
{
	pc++;
	return *opcode_ptr++;
}

INLINE UINT16 FETCHD16(void)
{
	UINT16 d;
	d = opcode_ptr[0] | (opcode_ptr[1] << 8);
	opcode_ptr += 2;
	pc += 2;
	return d;
}

INLINE UINT32 FETCHD32(void)
{
	UINT32 d;
	d = opcode_ptr[0] | (opcode_ptr[1] << 8) | (opcode_ptr[2] << 16) | (opcode_ptr[3] << 24);
	opcode_ptr += 4;
	pc += 4;
	return d;
}

static char *hexstring(UINT32 value, int digits)
{
	static char buffer[20];
	buffer[0] = '0';
	if (digits)
		snprintf(&buffer[1], 64, "%0*Xh", digits, value);
	else
		snprintf(&buffer[1], 64, "%Xh", value);
	return (buffer[1] >= '0' && buffer[1] <= '9') ? &buffer[1] : &buffer[0];
}

static char *shexstring(UINT32 value, int digits, int always)
{
	static char buffer[20];
	if (value >= 0x80000000)
		snprintf(buffer, 64, "-%s", hexstring(-value, digits));
	else if (always)
		snprintf(buffer, 64, "+%s", hexstring(value, digits));
	else
		return hexstring(value, digits);
	return buffer;
}

static char* handle_sib_byte( char* s, UINT8 mod )
{
	UINT32 i32;
	UINT8 scale, i, base;
	UINT8 sib = FETCHD();
	scale = (sib >> 6) & 0x3;
	i = (sib >> 3) & 0x7;
	base = sib & 0x7;

	switch( base )
	{
		case 0: s += snprintf( s, 64, "eax"); break;
		case 1: s += snprintf( s, 64, "ecx"); break;
		case 2: s += snprintf( s, 64, "edx"); break;
		case 3: s += snprintf( s, 64, "ebx"); break;
		case 4: s += snprintf( s, 64, "esp"); break;
		case 5:
			if( mod == 0 ) {
				i32 = FETCH32();
				s += snprintf( s, 64, "%s", hexstring(i32, 0) );
			} else if( mod == 1 ) {
				s += snprintf( s, 64, "ebp" );
			} else if( mod == 2 ) {
				s += snprintf( s, 64, "ebp" );
			}
			break;
		case 6: s += snprintf( s, 64, "esi"); break;
		case 7: s += snprintf( s, 64, "edi"); break;
	}
	if (scale)
	{
		switch( i )
		{
			case 0: s += snprintf( s, 64, "+eax*%d", (1 << scale)); break;
			case 1: s += snprintf( s, 64, "+ecx*%d", (1 << scale)); break;
			case 2: s += snprintf( s, 64, "+edx*%d", (1 << scale)); break;
			case 3: s += snprintf( s, 64, "+ebx*%d", (1 << scale)); break;
			case 4: break;
			case 5: s += snprintf( s, 64, "+ebp*%d", (1 << scale)); break;
			case 6: s += snprintf( s, 64, "+esi*%d", (1 << scale)); break;
			case 7: s += snprintf( s, 64, "+edi*%d", (1 << scale)); break;
		}
	}
	else
	{
		switch( i )
		{
			case 0: s += snprintf( s, 64, "+eax"); break;
			case 1: s += snprintf( s, 64, "+ecx"); break;
			case 2: s += snprintf( s, 64, "+edx"); break;
			case 3: s += snprintf( s, 64, "+ebx"); break;
			case 4: break;
			case 5: s += snprintf( s, 64, "+ebp"); break;
			case 6: s += snprintf( s, 64, "+esi"); break;
			case 7: s += snprintf( s, 64, "+edi"); break;
		}
	}
	return s;
}

static void handle_modrm(char* s)
{
	INT8 disp8;
	INT16 disp16;
	INT32 disp32;
	UINT8 mod, rm;

	modrm = FETCHD();
	mod = (modrm >> 6) & 0x3;
	rm = modrm & 0x7;

	if( modrm >= 0xc0 )
		return;

	switch(segment)
	{
		case SEG_CS: s += snprintf( s, 64, "cs:" ); break;
		case SEG_DS: s += snprintf( s, 64, "ds:" ); break;
		case SEG_ES: s += snprintf( s, 64, "es:" ); break;
		case SEG_FS: s += snprintf( s, 64, "fs:" ); break;
		case SEG_GS: s += snprintf( s, 64, "gs:" ); break;
		case SEG_SS: s += snprintf( s, 64, "ss:" ); break;
	}

	s += snprintf( s, 64, "[" );
	if( address_size ) {
		switch( rm )
		{
			case 0: s += snprintf( s, 64, "eax" ); break;
			case 1: s += snprintf( s, 64, "ecx" ); break;
			case 2: s += snprintf( s, 64, "edx" ); break;
			case 3: s += snprintf( s, 64, "ebx" ); break;
			case 4: s = handle_sib_byte( s, mod ); break;
			case 5:
				if( mod == 0 ) {
					disp32 = FETCHD32();
					s += snprintf( s, 64, "%s", hexstring(disp32, 0) );
				} else {
					s += snprintf( s, 64, "ebp" );
				}
				break;
			case 6: s += snprintf( s, 64, "esi" ); break;
			case 7: s += snprintf( s, 64, "edi" ); break;
		}
		if( mod == 1 ) {
			disp8 = FETCHD();
			s += snprintf( s, 64, "%s", shexstring((INT32)disp8, 0, TRUE) );
		} else if( mod == 2 ) {
			disp32 = FETCHD32();
			s += snprintf( s, 64, "%s", shexstring(disp32, 0, TRUE) );
		}
	} else {
		switch( rm )
		{
			case 0: s += snprintf( s, 64, "bx+si" ); break;
			case 1: s += snprintf( s, 64, "bx+di" ); break;
			case 2: s += snprintf( s, 64, "bx+si" ); break;
			case 3: s += snprintf( s, 64, "bx+di" ); break;
			case 4: s += snprintf( s, 64, "si" ); break;
			case 5: s += snprintf( s, 64, "di" ); break;
			case 6:
				if( mod == 0 ) {
					disp16 = FETCHD16();
					s += snprintf( s, 64, "%s", hexstring((unsigned) (UINT16) disp16, 0) );
				} else {
					s += snprintf( s, 64, "bp" );
				}
				break;
			case 7: s += snprintf( s, 64, "bx" ); break;
		}
		if( mod == 1 ) {
			disp8 = FETCHD();
			s += snprintf( s, 64, "%s", shexstring((INT32)disp8, 0, TRUE) );
		} else if( mod == 2 ) {
			disp16 = FETCHD16();
			s += snprintf( s, 64, "%s", shexstring((INT32)disp16, 0, TRUE) );
		}
	}
	s += snprintf( s, 64, "]" );
}

static char* handle_param(char* s, UINT32 param)
{
	UINT8 i8;
	UINT16 i16;
	UINT32 i32;
	UINT16 ptr;
	UINT32 addr;
	INT8 d8;
	INT16 d16;
	INT32 d32;

	switch(param)
	{
		case PARAM_REG:
			s += snprintf( s, 64, "%s", i386_reg[operand_size][MODRM_REG1] );
			break;

		case PARAM_REG8:
			s += snprintf( s, 64, "%s", i386_reg8[MODRM_REG1] );
			break;

		case PARAM_REG16:
			s += snprintf( s, 64, "%s", i386_reg[0][MODRM_REG1] );
			break;

		case PARAM_REG2_32:
			s += snprintf( s, 64, "%s", i386_reg[1][MODRM_REG2] );
			break;

		case PARAM_RM:
			if( modrm >= 0xc0 ) {
				s += snprintf( s, 64, "%s", i386_reg[operand_size][MODRM_REG2] );
			} else {
				if( operand_size )
					s += snprintf( s, 64, "dword ptr " );
				else
					s += snprintf( s, 64, "word ptr " );
				s += snprintf( s, 64, "%s", modrm_string );
			}
			break;

		case PARAM_RM8:
			if( modrm >= 0xc0 ) {
				s += snprintf( s, 64, "%s", i386_reg8[MODRM_REG2] );
			} else {
				s += snprintf( s, 64, "byte ptr " );
				s += snprintf( s, 64, "%s", modrm_string );
			}
			break;

		case PARAM_RM16:
			if( modrm >= 0xc0 ) {
				s += snprintf( s, 64, "%s", i386_reg[0][MODRM_REG2] );
			} else {
				s += snprintf( s, 64, "word ptr " );
				s += snprintf( s, 64, "%s", modrm_string );
			}
			break;

		case PARAM_I8:
			i8 = FETCHD();
			s += snprintf( s, 64, "%s", shexstring((INT8)i8, 0, FALSE) );
			break;

		case PARAM_I16:
			i16 = FETCHD16();
			s += snprintf( s, 64, "%s", shexstring((INT16)i16, 0, FALSE) );
			break;

		case PARAM_UI8:
			i8 = FETCHD();
			s += snprintf( s, 64, "%s", shexstring((UINT8)i8, 0, FALSE) );
			break;

		case PARAM_UI16:
			i16 = FETCHD16();
			s += snprintf( s, 64, "%s", shexstring((UINT16)i16, 0, FALSE) );
			break;

		case PARAM_IMM:
			if( operand_size ) {
				i32 = FETCHD32();
				s += snprintf( s, 64, "%s", hexstring(i32, 0) );
			} else {
				i16 = FETCHD16();
				s += snprintf( s, 64, "%s", hexstring(i16, 0) );
			}
			break;

		case PARAM_ADDR:
			if( operand_size ) {
				addr = FETCHD32();
				ptr = FETCHD16();
				s += snprintf( s, 64, "%s:", hexstring(ptr, 4) );
				s += snprintf( s, 64, "%s", hexstring(addr, 0) );
			} else {
				addr = FETCHD16();
				ptr = FETCHD16();
				s += snprintf( s, 64, "%s:", hexstring(ptr, 4) );
				s += snprintf( s, 64, "%s", hexstring(addr, 0) );
			}
			break;

		case PARAM_REL:
			if( operand_size ) {
				d32 = FETCHD32();
				s += snprintf( s, 64, "%s", hexstring(pc + d32, 0) );
			} else {
				/* make sure to keep the relative offset within the segment */
				d16 = FETCHD16();
				s += snprintf( s, 64, "%s", hexstring((pc & 0xFFFF0000) | ((pc + d16) & 0x0000FFFF), 0) );
			}
			break;

		case PARAM_REL8:
			d8 = FETCHD();
			s += snprintf( s, 64, "%s", hexstring(pc + d8, 0) );
			break;

		case PARAM_MEM_OFFS_B:
			d8 = FETCHD();
			s += snprintf( s, 64, "[%s]", hexstring(d8, 0) );
			break;

		case PARAM_MEM_OFFS_V:
			if( address_size ) {
				d32 = FETCHD32();
				s += snprintf( s, 64, "[%s]", hexstring(d32, 0) );
			} else {
				d32 = FETCHD16();
				s += snprintf( s, 64, "[%s]", hexstring(d32, 0) );
			}
			break;

		case PARAM_SREG:
			s += snprintf( s, 64, "%s", i386_sreg[MODRM_REG1] );
			break;

		case PARAM_CREG:
			s += snprintf( s, 64, "cr%d", MODRM_REG1 );
			break;

		case PARAM_TREG:
			s += snprintf( s, 64, "tr%d", MODRM_REG1 );
			break;

		case PARAM_1:
			s += snprintf( s, 64, "1" );
			break;

		case PARAM_DX:
			s += snprintf( s, 64, "dx" );
			break;

		case PARAM_AL: s += snprintf( s, 64, "al" ); break;
		case PARAM_CL: s += snprintf( s, 64, "cl" ); break;
		case PARAM_DL: s += snprintf( s, 64, "dl" ); break;
		case PARAM_BL: s += snprintf( s, 64, "bl" ); break;
		case PARAM_AH: s += snprintf( s, 64, "ah" ); break;
		case PARAM_CH: s += snprintf( s, 64, "ch" ); break;
		case PARAM_DH: s += snprintf( s, 64, "dh" ); break;
		case PARAM_BH: s += snprintf( s, 64, "bh" ); break;

		case PARAM_EAX: s += snprintf( s, 64, "%s", i386_reg[operand_size][0] ); break;
		case PARAM_ECX: s += snprintf( s, 64, "%s", i386_reg[operand_size][1] ); break;
		case PARAM_EDX: s += snprintf( s, 64, "%s", i386_reg[operand_size][2] ); break;
		case PARAM_EBX: s += snprintf( s, 64, "%s", i386_reg[operand_size][3] ); break;
		case PARAM_ESP: s += snprintf( s, 64, "%s", i386_reg[operand_size][4] ); break;
		case PARAM_EBP: s += snprintf( s, 64, "%s", i386_reg[operand_size][5] ); break;
		case PARAM_ESI: s += snprintf( s, 64, "%s", i386_reg[operand_size][6] ); break;
		case PARAM_EDI: s += snprintf( s, 64, "%s", i386_reg[operand_size][7] ); break;
	}
	return s;
}

static void handle_fpu(char *s, UINT8 op1, UINT8 op2)
{
	switch (op1 & 0x7)
	{
		case 0:		// Group D8
		{
			if (op2 < 0xc0)
			{
				pc--;		// adjust fetch pointer, so modrm byte read again
				opcode_ptr--;
				handle_modrm( modrm_string );
				switch ((op2 >> 3) & 0x7)
				{
					case 0: snprintf(s, 64, "fadd    dword ptr %s", modrm_string); break;
					case 1: snprintf(s, 64, "fmul    dword ptr %s", modrm_string); break;
					case 2: snprintf(s, 64, "fcom    dword ptr %s", modrm_string); break;
					case 3: snprintf(s, 64, "fcomp   dword ptr %s", modrm_string); break;
					case 4: snprintf(s, 64, "fsub    dword ptr %s", modrm_string); break;
					case 5: snprintf(s, 64, "fsubr   dword ptr %s", modrm_string); break;
					case 6: snprintf(s, 64, "fdiv    dword ptr %s", modrm_string); break;
					case 7: snprintf(s, 64, "fdivr   dword ptr %s", modrm_string); break;
				}
			}
			else
			{
				switch ((op2 >> 3) & 0x7)
				{
					case 0: snprintf(s, 64, "fadd    st(0),st(%d)", op2 & 0x7); break;
					case 1: snprintf(s, 64, "fcom    st(0),st(%d)", op2 & 0x7); break;
					case 2: snprintf(s, 64, "fsub    st(0),st(%d)", op2 & 0x7); break;
					case 3: snprintf(s, 64, "fdiv    st(0),st(%d)", op2 & 0x7); break;
					case 4: snprintf(s, 64, "fmul    st(0),st(%d)", op2 & 0x7); break;
					case 5: snprintf(s, 64, "fcomp   st(0),st(%d)", op2 & 0x7); break;
					case 6: snprintf(s, 64, "fsubr   st(0),st(%d)", op2 & 0x7); break;
					case 7: snprintf(s, 64, "fdivr   st(0),st(%d)", op2 & 0x7); break;
				}
			}
			break;
		}

		case 1:		// Group D9
		{
			if (op2 < 0xc0)
			{
				pc--;		// adjust fetch pointer, so modrm byte read again
				opcode_ptr--;
				handle_modrm( modrm_string );
				switch ((op2 >> 3) & 0x7)
				{
					case 0: snprintf(s, 64, "fld     dword ptr %s", modrm_string); break;
					case 1: snprintf(s, 64, "??? (FPU)"); break;
					case 2: snprintf(s, 64, "fst     dword ptr %s", modrm_string); break;
					case 3: snprintf(s, 64, "fstp    dword ptr %s", modrm_string); break;
					case 4: snprintf(s, 64, "fldenv  word ptr %s", modrm_string); break;
					case 5: snprintf(s, 64, "fldcw   word ptr %s", modrm_string); break;
					case 6: snprintf(s, 64, "fstenv  word ptr %s", modrm_string); break;
					case 7: snprintf(s, 64, "fstcw   word ptr %s", modrm_string); break;
				}
			}
			else
			{
				switch (op2 & 0x3f)
				{
					case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x06: case 0x07:
						snprintf(s, 64, "fld     st(0),st(%d)", op2 & 0x7); break;

					case 0x08: case 0x09: case 0x0a: case 0x0b: case 0x0c: case 0x0d: case 0x0e: case 0x0f:
						snprintf(s, 64, "fxch    st(0),st(%d)", op2 & 0x7); break;

					case 0x10: snprintf(s, 64, "fnop"); break;
					case 0x20: snprintf(s, 64, "fchs"); break;
					case 0x21: snprintf(s, 64, "fabs"); break;
					case 0x24: snprintf(s, 64, "ftst"); break;
					case 0x25: snprintf(s, 64, "fxam"); break;
					case 0x28: snprintf(s, 64, "fld1"); break;
					case 0x29: snprintf(s, 64, "fldl2t"); break;
					case 0x2a: snprintf(s, 64, "fldl2e"); break;
					case 0x2b: snprintf(s, 64, "fldpi"); break;
					case 0x2c: snprintf(s, 64, "fldlg2"); break;
					case 0x2d: snprintf(s, 64, "fldln2"); break;
					case 0x2e: snprintf(s, 64, "fldz"); break;
					case 0x30: snprintf(s, 64, "f2xm1"); break;
					case 0x31: snprintf(s, 64, "fyl2x"); break;
					case 0x32: snprintf(s, 64, "fptan"); break;
					case 0x33: snprintf(s, 64, "fpatan"); break;
					case 0x34: snprintf(s, 64, "fxtract"); break;
					case 0x35: snprintf(s, 64, "fprem1"); break;
					case 0x36: snprintf(s, 64, "fdecstp"); break;
					case 0x37: snprintf(s, 64, "fincstp"); break;
					case 0x38: snprintf(s, 64, "fprem"); break;
					case 0x39: snprintf(s, 64, "fyl2xp1"); break;
					case 0x3a: snprintf(s, 64, "fsqrt"); break;
					case 0x3b: snprintf(s, 64, "fsincos"); break;
					case 0x3c: snprintf(s, 64, "frndint"); break;
					case 0x3d: snprintf(s, 64, "fscale"); break;
					case 0x3e: snprintf(s, 64, "fsin"); break;
					case 0x3f: snprintf(s, 64, "fcos"); break;

					default: snprintf(s, 64, "??? (FPU)"); break;
				}
			}
			break;
		}

		case 2:		// Group DA
		{
			if (op2 < 0xc0)
			{
				pc--;		// adjust fetch pointer, so modrm byte read again
				opcode_ptr--;
				handle_modrm( modrm_string );
				switch ((op2 >> 3) & 0x7)
				{
					case 0: snprintf(s, 64, "fiadd   dword ptr %s", modrm_string); break;
					case 1: snprintf(s, 64, "fimul   dword ptr %s", modrm_string); break;
					case 2: snprintf(s, 64, "ficom   dword ptr %s", modrm_string); break;
					case 3: snprintf(s, 64, "ficomp  dword ptr %s", modrm_string); break;
					case 4: snprintf(s, 64, "fisub   dword ptr %s", modrm_string); break;
					case 5: snprintf(s, 64, "fisubr  dword ptr %s", modrm_string); break;
					case 6: snprintf(s, 64, "fidiv   dword ptr %s", modrm_string); break;
					case 7: snprintf(s, 64, "fidivr  dword ptr %s", modrm_string); break;
				}
			}
			else
			{
				switch (op2 & 0x3f)
				{
					case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x06: case 0x07:
						snprintf(s, 64, "fcmovb  st(0),st(%d)", op2 & 0x7); break;

					case 0x08: case 0x09: case 0x0a: case 0x0b: case 0x0c: case 0x0d: case 0x0e: case 0x0f:
						snprintf(s, 64, "fcmove  st(0),st(%d)", op2 & 0x7); break;

					case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x16: case 0x17:
						snprintf(s, 64, "fcmovbe st(0),st(%d)", op2 & 0x7); break;

					case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1c: case 0x1d: case 0x1e: case 0x1f:
						snprintf(s, 64, "fcmovu  st(0),st(%d)", op2 & 0x7); break;

					default: snprintf(s, 64, "??? (FPU)"); break;

				}
			}
			break;
		}

		case 3:		// Group DB
		{
			if (op2 < 0xc0)
			{
				pc--;		// adjust fetch pointer, so modrm byte read again
				opcode_ptr--;
				handle_modrm( modrm_string );
				switch ((op2 >> 3) & 0x7)
				{
					case 0: snprintf(s, 64, "fild    dword ptr %s", modrm_string); break;
					case 1: snprintf(s, 64, "??? (FPU)"); break;
					case 2: snprintf(s, 64, "fist    dword ptr %s", modrm_string); break;
					case 3: snprintf(s, 64, "fistp   dword ptr %s", modrm_string); break;
					case 4: snprintf(s, 64, "??? (FPU)"); break;
					case 5: snprintf(s, 64, "fld     tword ptr %s", modrm_string); break;
					case 6: snprintf(s, 64, "??? (FPU)"); break;
					case 7: snprintf(s, 64, "fstp    tword ptr %s", modrm_string); break;
				}
			}
			else
			{
				switch (op2 & 0x3f)
				{
					case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x06: case 0x07:
						snprintf(s, 64, "fcmovnb st(0),st(%d)", op2 & 0x7); break;

					case 0x08: case 0x09: case 0x0a: case 0x0b: case 0x0c: case 0x0d: case 0x0e: case 0x0f:
						snprintf(s, 64, "fcmovne st(0),st(%d)", op2 & 0x7); break;

					case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x16: case 0x17:
						snprintf(s, 64, "fcmovnbe st(0),st(%d)", op2 & 0x7); break;

					case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1c: case 0x1d: case 0x1e: case 0x1f:
						snprintf(s, 64, "fcmovnu st(0),st(%d)", op2 & 0x7); break;

					case 0x22: snprintf(s, 64, "fclex"); break;
					case 0x23: snprintf(s, 64, "finit"); break;

					case 0x28: case 0x29: case 0x2a: case 0x2b: case 0x2c: case 0x2d: case 0x2e: case 0x2f:
						snprintf(s, 64, "fucomi  st(0),st(%d)", op2 & 0x7); break;

					case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x36: case 0x37:
						snprintf(s, 64, "fcomi   st(0),st(%d)", op2 & 0x7); break;

					default: snprintf(s, 64, "??? (FPU)"); break;
				}
			}
			break;
		}

		case 4:		// Group DC
		{
			if (op2 < 0xc0)
			{
				pc--;		// adjust fetch pointer, so modrm byte read again
				opcode_ptr--;
				handle_modrm( modrm_string );
				switch ((op2 >> 3) & 0x7)
				{
					case 0: snprintf(s, 64, "fadd    qword ptr %s", modrm_string); break;
					case 1: snprintf(s, 64, "fmul    qword ptr %s", modrm_string); break;
					case 2: snprintf(s, 64, "fcom    qword ptr %s", modrm_string); break;
					case 3: snprintf(s, 64, "fcomp   qword ptr %s", modrm_string); break;
					case 4: snprintf(s, 64, "fsub    qword ptr %s", modrm_string); break;
					case 5: snprintf(s, 64, "fsubr   qword ptr %s", modrm_string); break;
					case 6: snprintf(s, 64, "fdiv    qword ptr %s", modrm_string); break;
					case 7: snprintf(s, 64, "fdivr   qword ptr %s", modrm_string); break;
				}
			}
			else
			{
				switch (op2 & 0x3f)
				{
					case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x06: case 0x07:
						snprintf(s, 64, "fadd    st(%d),st(0)", op2 & 0x7); break;

					case 0x08: case 0x09: case 0x0a: case 0x0b: case 0x0c: case 0x0d: case 0x0e: case 0x0f:
						snprintf(s, 64, "fmul    st(%d),st(0)", op2 & 0x7); break;

					case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x26: case 0x27:
						snprintf(s, 64, "fsubr   st(%d),st(0)", op2 & 0x7); break;

					case 0x28: case 0x29: case 0x2a: case 0x2b: case 0x2c: case 0x2d: case 0x2e: case 0x2f:
						snprintf(s, 64, "fsub    st(%d),st(0)", op2 & 0x7); break;

					case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x36: case 0x37:
						snprintf(s, 64, "fdivr   st(%d),st(0)", op2 & 0x7); break;

					case 0x38: case 0x39: case 0x3a: case 0x3b: case 0x3c: case 0x3d: case 0x3e: case 0x3f:
						snprintf(s, 64, "fdiv    st(%d),st(0)", op2 & 0x7); break;

					default: snprintf(s, 64, "??? (FPU)"); break;
				}
			}
			break;
		}

		case 5:		// Group DD
		{
			if (op2 < 0xc0)
			{
				pc--;		// adjust fetch pointer, so modrm byte read again
				opcode_ptr--;
				handle_modrm( modrm_string );
				switch ((op2 >> 3) & 0x7)
				{
					case 0: snprintf(s, 64, "fld     qword ptr %s", modrm_string); break;
					case 1: snprintf(s, 64, "??? (FPU)"); break;
					case 2: snprintf(s, 64, "fst     qword ptr %s", modrm_string); break;
					case 3: snprintf(s, 64, "fstp    qword ptr %s", modrm_string); break;
					case 4: snprintf(s, 64, "frstor  %s", modrm_string); break;
					case 5: snprintf(s, 64, "??? (FPU)"); break;
					case 6: snprintf(s, 64, "fsave   %s", modrm_string); break;
					case 7: snprintf(s, 64, "fstsw   word ptr %s", modrm_string); break;
				}
			}
			else
			{
				switch (op2 & 0x3f)
				{
					case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x06: case 0x07:
						snprintf(s, 64, "ffree   st(%d)", op2 & 0x7); break;

					case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x16: case 0x17:
						snprintf(s, 64, "fst     st(%d)", op2 & 0x7); break;

					case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1c: case 0x1d: case 0x1e: case 0x1f:
						snprintf(s, 64, "fstp    st(%d)", op2 & 0x7); break;

					case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x26: case 0x27:
						snprintf(s, 64, "fucom   st(%d), st(0)", op2 & 0x7); break;

					case 0x28: case 0x29: case 0x2a: case 0x2b: case 0x2c: case 0x2d: case 0x2e: case 0x2f:
						snprintf(s, 64, "fucomp  st(%d)", op2 & 0x7); break;

					default: snprintf(s, 64, "??? (FPU)"); break;
				}
			}
			break;
		}

		case 6:		// Group DE
		{
			if (op2 < 0xc0)
			{
				pc--;		// adjust fetch pointer, so modrm byte read again
				opcode_ptr--;
				handle_modrm( modrm_string );
				switch ((op2 >> 3) & 0x7)
				{
					case 0: snprintf(s, 64, "fiadd   word ptr %s", modrm_string); break;
					case 1: snprintf(s, 64, "fimul   word ptr %s", modrm_string); break;
					case 2: snprintf(s, 64, "ficom   word ptr %s", modrm_string); break;
					case 3: snprintf(s, 64, "ficomp  word ptr %s", modrm_string); break;
					case 4: snprintf(s, 64, "fisub   word ptr %s", modrm_string); break;
					case 5: snprintf(s, 64, "fisubr  word ptr %s", modrm_string); break;
					case 6: snprintf(s, 64, "fidiv   word ptr %s", modrm_string); break;
					case 7: snprintf(s, 64, "fidivr  word ptr %s", modrm_string); break;
				}
			}
			else
			{
				switch (op2 & 0x3f)
				{
					case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x06: case 0x07:
						snprintf(s, 64, "faddp   st(%d)", op2 & 0x7); break;

					case 0x08: case 0x09: case 0x0a: case 0x0b: case 0x0c: case 0x0d: case 0x0e: case 0x0f:
						snprintf(s, 64, "fmulp   st(%d)", op2 & 0x7); break;

					case 0x19: snprintf(s, 64, "fcompp"); break;

					case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x26: case 0x27:
						snprintf(s, 64, "fsubrp  st(%d)", op2 & 0x7); break;

					case 0x28: case 0x29: case 0x2a: case 0x2b: case 0x2c: case 0x2d: case 0x2e: case 0x2f:
						snprintf(s, 64, "fsubp   st(%d)", op2 & 0x7); break;

					case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x36: case 0x37:
						snprintf(s, 64, "fdivrp  st(%d), st(0)", op2 & 0x7); break;

					case 0x38: case 0x39: case 0x3a: case 0x3b: case 0x3c: case 0x3d: case 0x3e: case 0x3f:
						snprintf(s, 64, "fdivp   st(%d)", op2 & 0x7); break;

					default: snprintf(s, 64, "??? (FPU)"); break;
				}
			}
			break;
		}

		case 7:		// Group DF
		{
			if (op2 < 0xc0)
			{
				pc--;		// adjust fetch pointer, so modrm byte read again
				opcode_ptr--;
				handle_modrm( modrm_string );
				switch ((op2 >> 3) & 0x7)
				{
					case 0: snprintf(s, 64, "fild    word ptr %s", modrm_string); break;
					case 1: snprintf(s, 64, "??? (FPU)"); break;
					case 2: snprintf(s, 64, "fist    word ptr %s", modrm_string); break;
					case 3: snprintf(s, 64, "fistp   word ptr %s", modrm_string); break;
					case 4: snprintf(s, 64, "fbld    %s", modrm_string); break;
					case 5: snprintf(s, 64, "fild    qword ptr %s", modrm_string); break;
					case 6: snprintf(s, 64, "fbstp   %s", modrm_string); break;
					case 7: snprintf(s, 64, "fistp   qword ptr %s", modrm_string); break;
				}
			}
			else
			{
				switch (op2 & 0x3f)
				{
					case 0x20: snprintf(s, 64, "fstsw   ax"); break;

					case 0x28: case 0x29: case 0x2a: case 0x2b: case 0x2c: case 0x2d: case 0x2e: case 0x2f:
						snprintf(s, 64, "fucomip st(%d)", op2 & 0x7); break;

					case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x36: case 0x37:
						snprintf(s, 64, "fcomip  st(%d),st(0)", op2 & 0x7); break;

					default: snprintf(s, 64, "??? (FPU)"); break;
				}
			}
			break;
		}
	}
}

static void decode_opcode(char *s, const I386_OPCODE *op, UINT8 op1)
{
	int i;
	UINT8 op2;

	switch( op->flags )
	{
		case OP_SIZE:
			operand_size ^= 1;
			op2 = FETCH();
			decode_opcode( s, &opcode_table1[op2], op1 );
			return;

		case ADDR_SIZE:
			address_size ^= 1;
			op2 = FETCH();
			decode_opcode( s, &opcode_table1[op2], op1 );
			return;

		case TWO_BYTE:
			op2 = FETCHD();
			decode_opcode( s, &opcode_table2[op2], op1 );
			return;

		case SEG_CS:
		case SEG_DS:
		case SEG_ES:
		case SEG_FS:
		case SEG_GS:
		case SEG_SS:
			segment = op->flags;
			op2 = FETCH();
			decode_opcode( s, &opcode_table1[op2], op1 );
			return;

		case PREFIX:
			s += snprintf( s, 64, "%-8s", op->mnemonic );
			op2 = FETCH();
			decode_opcode( s, &opcode_table1[op2], op1 );
			return;

		case VAR_NAME:
			if( operand_size ) {
				int p=0;
				while(op->mnemonic[p] != 0)
				{
					p++;
				};
				s += snprintf( s, 64, "%-8s", &op->mnemonic[p+1] );
			} else {
				s += snprintf( s, 64, "%-8s", op->mnemonic );
			}
			goto handle_params;

		case GROUP:
			handle_modrm( modrm_string );
			for( i=0; i < (sizeof(group_op_table) / sizeof(GROUP_OP)); i++ ) {
				if( mame_stricmp(op->mnemonic, group_op_table[i].mnemonic) == 0 ) {
					decode_opcode( s, &group_op_table[i].opcode[MODRM_REG1], op1 );
					return;
				}
			}
			goto handle_unknown;

		case FPU:
			op2 = FETCHD();
			handle_fpu( s, op1, op2);
			return;

		case MODRM:
			handle_modrm( modrm_string );
			break;
	}

	s += snprintf( s, 64, "%-8s", op->mnemonic );
	dasm_flags = op->dasm_flags;

handle_params:
	if( op->param1 != 0 ) {
		s = handle_param( s, op->param1 );
	}

	if( op->param2 != 0 ) {
		s += snprintf( s, 64, "," );
		s = handle_param( s, op->param2 );
	}

	if( op->param3 != 0 ) {
		s += snprintf( s, 64, "," );
		s = handle_param( s, op->param3 );
	}
	return;

handle_unknown:
	snprintf(s, 64, "???");
}

int i386_dasm_one(char *buffer, UINT32 eip, UINT8 *oprom, int addr_size, int op_size)
{
	UINT8 op;

	opcode_ptr = oprom;
	opcode_table1 = i386_opcode_table1;
	opcode_table2 = i386_opcode_table2;
	address_size = addr_size;
	operand_size = op_size;
	pc = eip;
	dasm_flags = 0;
	segment = 0;

	op = FETCH();

	decode_opcode( buffer, &opcode_table1[op], op );
	return (pc-eip) | dasm_flags | DASMFLAG_SUPPORTED;
}

int necv_dasm_one(char *buffer, UINT32 eip, UINT8 *oprom, int addr_size, int op_size)
{
	UINT8 op;

	opcode_ptr = oprom;
	opcode_table1 = i386_opcode_table1;
	opcode_table2 = necv_opcode_table2;
	address_size = addr_size;
	operand_size = op_size;
	pc = eip;
	dasm_flags = 0;
	segment = 0;

	op = FETCH();

	decode_opcode( buffer, &opcode_table1[op], op );
	return (pc-eip) | dasm_flags | DASMFLAG_SUPPORTED;
}
