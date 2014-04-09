#ifndef _MACRO_H_
#define _MACRO_H_
#include <stdio.h>

#define NR_CPUS 	2



#define atomic_t int
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



#endif
