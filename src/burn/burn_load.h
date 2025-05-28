#ifndef BURN_LOAD_H
#define BURN_LOAD_H

// ROM loading flags
#define LD_NIBBLES      (1<<0)
#define LD_INVERT       (1<<1)
#define LD_BYTESWAP     (1<<2)
#define LD_BYTESWAP_END (1<<3)
#define LD_REVERSE      (1<<4)
#define LD_XOR          (1<<7)
#define LD_GROUP_MANY   (1<<15)

// Group size macros
#define LD_GROUP(a)     (((a) & 15) << 8)
#define LD_GROUPSIZE(a) (((a) >> 8) & 15)

#endif // BURN_LOAD_H 