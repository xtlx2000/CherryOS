#ifndef _MACRO_H_
#define _MACRO_H_
#include <stdio.h>
#include "list.h"

#define NR_CPUS 	2



#define atomic_t int
#define u8 unsigned char
#define u16 unsigned short
#define u32 unsigned int
#define u64 unsigned long


#define s8  unsigned int




#define pgoff_t int
#define gfp_t int

#define loff_t int

#define ssize_t unsigned long
#define size_t unsigned long




#define spinlock_t int


/*      
 * Generic compiler-dependent macros required for kernel
 * build go below this comment. Actual compiler/compiler version
 * specific implementations come from the above header files
 */

#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)



#define DEBUG 1
#define PRINT_DEBUG(fmt, args...)    if(DEBUG)printf(fmt, ## args)



struct __wait_queue_head {
    spinlock_t lock;
    struct list_head task_list;
};  
typedef struct __wait_queue_head wait_queue_head_t;


#define BITS_PER_LONG (sizeof(unsigned long))


#define USHORT_MAX  ((u16)(~0U))
#define SHORT_MAX   ((u16)(USHORT_MAX>>1))
#define SHORT_MIN   (-SHORT_MAX - 1)
#define INT_MAX     ((int)(~0U>>1))
#define INT_MIN     (-INT_MAX - 1)
#define UINT_MAX    (~0U)
#define LONG_MAX    ((long)(~0UL>>1))
#define LONG_MIN    (-LONG_MAX - 1)
#define ULONG_MAX   (~0UL)                                                                                  
#define LLONG_MAX   ((long long)(~0ULL>>1))
#define LLONG_MIN   (-LLONG_MAX - 1)
#define ULLONG_MAX  (~0ULL)



#define BUG() do { \
    PRINT_DEBUG("BUG: failure at %s:%d/%s()!\n", __FILE__, __LINE__, __FUNCTION__); \
} while (0)

#define BUG_ON(condition) do { if (unlikely(condition)) BUG(); } while(0)



#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))


#endif
