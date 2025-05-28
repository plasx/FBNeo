#include "m68kcpu.h"
#include "../../burn/cpu_generated/m68kops.h" 

/* ======================================================================== */
/* ========================= INSTRUCTION HANDLERS ========================== */
/* ======================================================================== */

/* Instruction handler function prototype */
#define M68KMAKE_PROTOTYPE_HEADER(MODE, NAME)
void NAME(void)
{
}

/* Master table entry */
#define M68KMAKE_TABLE_HEADER(OPCODE, NAME)

/* Generate a dynamic call to the instruction handler for this opcode */
#define M68KMAKE_OPCODE_HANDLER_HEADER(HANDLER_NAME)
/* Register to jump table */
#define M68KMAKE_OPCODE_HANDLER_FOOTER

/* Support for 68010+ extended opcodes */
#define M68KMAKE_OP(OPCODE, MASK, OPHANDLER_CPU, OPHANDLER_PARAMS) 