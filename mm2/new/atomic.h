#ifndef _ATOMIC_H_
#define _ATOMIC_H_






/**
 * atomic_read - read atomic variable
 * @v: pointer of type atomic_t
 *
 * Atomically reads the value of @v.
 */
#define atomic_read(v)      ((v)->counter)

/**
 * atomic_set - set atomic variable
 * @v: pointer of type atomic_t
 * @i: required value
 *
 * Atomically sets the value of @v to @i.
 */
#define atomic_set(v, i)    (((v)->counter) = (i))




#define atomic_inc(v) atomic_add(1,(v))

/*
 * To get proper branch prediction for the main line, we must branch
 * forward to code at the end of this object's .text section, then
 * branch back to restart the operation.
 */
void atomic_add(int i, atomic_t * v)                                                      
{
    unsigned long temp;
    __asm__ __volatile__(
    "1: ldl_l %0,%1\n"
    "   addl %0,%2,%0\n"
    "   stl_c %0,%1\n"
    "   beq %0,2f\n"
    ".subsection 2\n"
    "2: br 1b\n"
    ".previous"
    :"=&r" (temp), "=m" (v->counter)
    :"Ir" (i), "m" (v->counter));
}


void atomic_sub(int i, atomic_t * v)
{
    unsigned long temp;                                                                                     
    __asm__ __volatile__(
    "1: ldl_l %0,%1\n"
    "   subl %0,%2,%0\n"
    "   stl_c %0,%1\n"
    "   beq %0,2f\n"
    ".subsection 2\n"
    "2: br 1b\n"
    ".previous"
    :"=&r" (temp), "=m" (v->counter)
    :"Ir" (i), "m" (v->counter));
}



#endif
