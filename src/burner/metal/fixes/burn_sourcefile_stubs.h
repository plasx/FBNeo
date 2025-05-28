#ifndef _BURN_SOURCEFILE_STUBS_H_
#define _BURN_SOURCEFILE_STUBS_H_

#ifdef __cplusplus
extern "C" {
#endif

// Define the sourcefile table structure
typedef struct {
    const char* game_name;
    const char* sourcefile;
} sourcefile_entry;

// Declare the sourcefile table
extern sourcefile_entry sourcefile_table[];

#ifdef __cplusplus
}
#endif

#endif // _BURN_SOURCEFILE_STUBS_H_ 