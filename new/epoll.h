#ifndef _EPOLL_H_
#define _EPOLL_H_

/* 
 * structures and helpers for f_op->poll implementations
 */
typedef void (*poll_queue_proc)(struct file *, wait_queue_head_t *, struct poll_table_struct *);



typedef struct poll_table_struct {
    poll_queue_proc qproc;
} poll_table;


#endif
