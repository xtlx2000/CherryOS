#ifndef _SCHED_H_
#define _SCHED_H_

#include "pipe.h"

#define MAX_TASKS		1000	
#define TASK_GDT0		3		
#define MAX_TASKS_LV	100
#define MAX_TASKLEVELS	10

struct tss_struct {
	int backlink, 
		esp0, 
		ss0, 
		esp1, 
		ss1, 
		esp2, 
		ss2, 
		cr3;
	/* 32 bits registers*/
	int eip, 
		eflags, 
		eax,
		ecx, 
		edx, 
		ebx, 
		esp, 
		ebp, 
		esi, 
		edi;
	/* 16 bits registers */
	int es, 
		cs, 
		ss, 
		ds, 
		fs, 
		gs;
	/* task */
	int ldtr, 
		iomap;
};

struct task_struct {
	int sel, /* gdt */
		flags; 
	int level, 
		priority;
	struct pipe_struct pipe;
	struct tss_struct tss;
	struct terminal *cons;
	int ds_base;
};
struct task_level {
	int running; /* num of running tasks */
	int now; /* assigned current task */
	struct task_struct *tasks[MAX_TASKS_LV];
};
struct task_controller {
	int now_lv;
	char lv_change; /* if change LEVEL when next task is assigned */
	struct task_level level[MAX_TASKLEVELS];
	struct task_struct tasks0[MAX_TASKS];
};

extern struct timer_struct *task_timer;
struct mem_controller;
struct task_struct *task_now(void);
struct task_struct *task_init(struct mem_controller *memman);
struct task_struct *task_alloc(void);
void task_run(struct task_struct *task, int level, int priority);
void task_switch(void);
void task_sleep(struct task_struct *task);





#endif
