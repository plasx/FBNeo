#ifndef _BURN_STDFUNC_H
#define _BURN_STDFUNC_H

// Standard functions and macros for ROM and input handling

#ifdef __cplusplus
extern "C" {
#endif

// Standard ROM loading function macros
// Don't define RomInfo/RomName functions if they already exist
#define STD_ROM_FN(name)												\
static INT32 name##RomInfo(struct BurnRomInfo* pri, UINT32 i)			\
{																		\
	struct BurnRomInfo* por = (struct BurnRomInfo*)name##RomDesc + i;	\
	if (i >= sizeof(name##RomDesc) / sizeof(name##RomDesc[0])) {		\
		return 1;														\
	}																	\
	if (pri) {															\
		pri->nLen = por->nLen;											\
		pri->nCrc = por->nCrc;											\
		pri->nType = por->nType;										\
	}																	\
	return 0;															\
}																		\
																		\
static INT32 name##RomName(char** pszName, UINT32 i, INT32 nAka)		\
{																		\
	struct BurnRomInfo* por = (struct BurnRomInfo*)name##RomDesc + i;	\
	if (i >= sizeof(name##RomDesc) / sizeof(name##RomDesc[0])) {		\
		return 1;														\
	}																	\
	if (nAka) {															\
		return 1;														\
	}																	\
	*pszName = por->szName;												\
	return 0;															\
}

#define STDROMPICKEXT(name, rom1, rom2, rom3)							\
static struct BurnRomInfo name##RomDesc[] = {							\
	{ rom1, 0, 0, 0 },													\
	{ rom2, 0, 0, 0 },													\
	{ rom3, 0, 0, 0 },													\
};

// Standard ROM list definitions - don't attempt to redefine if already defined
#define STD_ROM_PICK(name)												\
struct name##_helper {}; /* Dummy type to handle cases where RomDesc already exists */

#define STD_ROM_END														\
};

// Standard input port definitions
#define STD_INPUT_PORTS_START(name)										\
static struct BurnInputInfo name##InputList[] = {

#define STD_INPUT_PORTS_END												\
};

// Standard input info macro
// This version is modified to not define InputList if it's already defined
#define STDINPUTINFO(Name)												\
UINT8 Name##_InputPort0[8] = {0, 0, 0, 0, 0, 0, 0, 0};                  \
UINT8 Name##_InputPort1[8] = {0, 0, 0, 0, 0, 0, 0, 0};                  \
UINT8 Name##_InputPort2[8] = {0, 0, 0, 0, 0, 0, 0, 0};                  \
UINT8 Name##_Dip[3] = {0, 0, 0};                                        \
UINT8 Name##_Reset = 0;                                                  \
                                                                         \
static INT32 Name##InputInfo(struct BurnInputInfo* pii, UINT32 i)        \
{                                                                        \
	if (i >= sizeof(Name##InputList) / sizeof(Name##InputList[0])) {    \
		return 1;                                                        \
	}                                                                    \
	if (pii) {                                                           \
		*pii = Name##InputList[i];                                       \
	}                                                                    \
	return 0;                                                            \
}

// Standard input info macro with extended info
#define STDINPUTINFOSPEC(Name, Info1)									\
static INT32 Name##InputInfo(struct BurnInputInfo* pii, UINT32 i)		\
{																		\
	if (i >= sizeof(Info1) / sizeof(Info1[0])) {						\
		return 1;														\
	}																	\
	if (pii) {															\
		*pii = Info1[i];												\
	}																	\
	return 0;															\
}

// More input extensions
#define STDINPUTINFOEXT(Name, Info1, Info2)									\
static INT32 Name##InputInfo(struct BurnInputInfo* pii, UINT32 i)			\
{																			\
	if (i >= sizeof(Info1##InputList) / sizeof(Info1##InputList[0])) {		\
		i -= sizeof(Info1##InputList) / sizeof(Info1##InputList[0]);		\
		if (i >= sizeof(Info2##InputList) / sizeof(Info2##InputList[0])) {	\
			return 1;														\
		}																	\
		if (pii) {															\
			*pii = Info2##InputList[i];										\
		}																	\
		return 0;															\
	}																		\
	if (pii) {																\
		*pii = Info1##InputList[i];											\
	}																		\
	return 0;																\
}

// Standard DIP info macro
#define STDDIPINFO(Name)												\
static struct BurnDIPInfo Name##DIPList[] = {							\
	{0x00,	0xFF, 0xFF,	0x00, NULL},									\
	{0x01,	0xFF, 0xFF,	0x00, NULL},									\
	{0x02,	0xFF, 0xFF,	0x00, NULL},									\
};

// Extended DIP info macro
#define STDDIPINFOEXT(Name, Info1, Info2)								\
static struct BurnDIPInfo Name##DIPList[] = {							\
	{0x00,	0xFF, 0xFF,	0x00, NULL},									\
	{0x01,	0xFF, 0xFF,	0x00, NULL},									\
	{0x02,	0xFF, 0xFF,	0x00, NULL},									\
	Info1																\
	Info2																\
};

// sample support
#define STD_SAMPLE_PICK(Name)											\
static struct BurnSampleInfo* Name##PickSample(UINT32 i)				\
{																		\
	if (i >= sizeof(Name##SampleDesc) / sizeof(Name##SampleDesc[0])) {	\
		return NULL;													\
	}																	\
	return Name##SampleDesc + i;										\
}

#define STD_SAMPLE_FN(Name)												\
static INT32 Name##SampleInfo(struct BurnSampleInfo* pri, UINT32 i)		\
{																		\
	struct BurnSampleInfo* por = Name##PickSample(i);					\
	if (por == NULL) {													\
		return 1;														\
	}																	\
	if (pri) {															\
		pri->nFlags = por->nFlags;										\
	}																	\
	return 0;															\
}																		\
																		\
static INT32 Name##SampleName(char** pszName, UINT32 i, INT32 nAka)		\
{											   		 					\
	struct BurnSampleInfo *por = Name##PickSample(i);					\
	if (por == NULL) {													\
		return 1;														\
	}																	\
	if (nAka) {															\
		return 1;														\
	}																	\
	*pszName = por->szName;												\
	return 0;															\
}

// hdd support
#define STD_HDD_PICK(Name)												\
static struct BurnHDDInfo* Name##PickHDD(UINT32 i)						\
{																		\
	if (i >= sizeof(Name##HDDDesc) / sizeof(Name##HDDDesc[0])) {		\
		return NULL;													\
	}																	\
	return Name##HDDDesc + i;											\
}

#define STD_HDD_FN(Name)												\
static INT32 Name##HDDInfo(struct BurnHDDInfo* pri, UINT32 i)			\
{																		\
	struct BurnHDDInfo* por = Name##PickHDD(i);							\
	if (por == NULL) {													\
		return 1;														\
	}																	\
	if (pri) {															\
		pri->nLen = por->nLen;											\
		pri->nCrc = por->nCrc;											\
	}																	\
	return 0;															\
}																		\
																		\
static INT32 Name##HDDName(char** pszName, UINT32 i, INT32 nAka)		\
{											   		 					\
	struct BurnHDDInfo *por = Name##PickHDD(i);							\
	if (por == NULL) {													\
		return 1;														\
	}																	\
	if (nAka) {															\
		return 1;														\
	}																	\
	*pszName = por->szName;												\
	return 0;															\
}

#ifdef __cplusplus
}
#endif

#endif // _BURN_STDFUNC_H
