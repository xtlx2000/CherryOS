#ifndef _MACRO_H_
#define _MACRO_H_


#define NR_CPUS 	2



#define atomic_t int
#define u16 unsigned short
#define u32 unsigned int



#define pgoff_t int
#define gfp_t int



#define spinlock_t int


/*      
 * Generic compiler-dependent macros required for kernel
 * build go below this comment. Actual compiler/compiler version
 * specific implementations come from the above header files
 */

#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)



#endif
