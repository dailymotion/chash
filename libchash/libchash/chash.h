// Consistent hashing library
// pyke@dailymotion.com - 05/2009

#ifndef __CHASH_INCLUDE
#define __CHASH_INCLUDE

#ifdef __cplusplus
extern "C"{
#endif

// Mandatory includes
#include <sys/types.h>

// Public defines
#define CHASH_ERROR_DONE                 (0)
#define CHASH_ERROR_MEMORY               (-1)
#define CHASH_ERROR_IO                   (-2)
#define CHASH_ERROR_INVALID_PARAMETER    (-10)
#define CHASH_ERROR_ALREADY_INITIALIZED  (-11)
#define CHASH_ERROR_NOT_INITIALIZED      (-12)
#define CHASH_ERROR_NOT_FOUND            (-13)

#pragma pack(push, 1)

typedef struct
{
    u_char       weight;
    char         *name;
} CHASH_TARGET;
typedef struct
{
    u_int32_t    hash;
    u_int16_t    target;
} CHASH_ITEM;
typedef struct
{
    u_int16_t    rank;
    u_int16_t    target;
} CHASH_LOOKUP;
typedef struct
{
    u_int32_t    magic;
    u_char       frozen;
    u_int16_t    targets_count;
    CHASH_TARGET *targets;
    u_int32_t    items_count;
    CHASH_ITEM   *continuum;
    CHASH_LOOKUP *lookups;
    char         **lookup;
} CHASH_CONTEXT;

#pragma pack(pop)

// Public API
int chash_initialize(CHASH_CONTEXT *, u_char);
int chash_terminate(CHASH_CONTEXT *, u_char);
int chash_add_target(CHASH_CONTEXT *, const char *, u_char);
int chash_remove_target(CHASH_CONTEXT *, const char *);
int chash_clear_targets(CHASH_CONTEXT *);
int chash_targets_count(CHASH_CONTEXT *);
int chash_serialize(CHASH_CONTEXT *, u_char **);
int chash_unserialize(CHASH_CONTEXT *, const u_char *, u_int32_t);
int chash_file_serialize(CHASH_CONTEXT *, const char *);
int chash_file_unserialize(CHASH_CONTEXT *, const char *);
int chash_lookup(CHASH_CONTEXT *, const char *, u_int16_t, char ***);
int chash_lookup_balance(CHASH_CONTEXT *, const char *, u_int16_t, char **);

#ifdef __cplusplus
}
#endif

#endif
