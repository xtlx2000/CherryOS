#ifndef _TIMER_H_
#define _TIMER_H_

#define MAX_TIMER		500

struct timer_struct {
	struct timer_struct *next;
	unsigned int timeout;
	char flags, flags2;
	struct pipe_struct *pipe;
	int data;
};
struct timer_controller {
	unsigned int count, next;
	struct timer_struct *t0;
	struct timer_struct timers0[MAX_TIMER];
};

struct pipe_struct;

extern struct timer_controller timerctl;
void init_pit(void);
struct timer_struct *timer_alloc(void);
void timer_free(struct timer_struct *timer);
void timer_init(struct timer_struct *timer, struct pipe_struct *pipe, int data);
void timer_settime(struct timer_struct *timer, unsigned int timeout);
void inthandler20(int *esp);
int timer_cancel(struct timer_struct *timer);
void timer_cancelall(struct pipe_struct *pipe);


#endif

