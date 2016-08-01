#ifndef types_h
#define types_h

typedef float f32;
typedef double f64;
 
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
 
typedef size_t memory_index;
 
#define kibi(n) ((n) << 10)
#define mebi(n) ((n) << 20)
#define gibi(n) ((n) << 30)
 
#define arrayCount(array) (sizeof(array) / sizeof( (array)[0] ))


#define allocArray(type, count) ((type*) malloc((count) * sizeof(type)))
 
#define global_variable static
#define local_persist static
 
#define invalidCodePath assert(!"Invalid code path")
 
#define tau 6.2831853072

#include <assert.h>

#endif