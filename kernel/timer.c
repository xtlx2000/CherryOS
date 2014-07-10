
#include "include/timer.h"
#include "include/sched.h"
#include "include/int.h"
#include "include/interaction.h"


#define PIT_CTRL	0x0043
#define PIT_CNT0	0x0040

struct timer_controller timerctl;

#define TIMER_FLAGS_ALLOC		1	
#define TIMER_FLAGS_USING		2	


void init_pit(void)
{
	int i;
	struct timer_struct *t;
	io_out8(PIT_CTRL, 0x34);
	io_out8(PIT_CNT0, 0x9c);
	io_out8(PIT_CNT0, 0x2e);
	timerctl.count = 0;
	for (i = 0; i < MAX_TIMER; i++) {
			timerctl.timers0[i].flags = 0; 
		}
	t = timer_alloc();
	t->timeout = 0xffffffff;
	t->flags = TIMER_FLAGS_USING;
	t->next = 0;
	timerctl.t0 = t; 
	timerctl.next = 0xffffffff;

	return;
}

struct timer_struct *timer_alloc(void)
{
	int i;
	for (i = 0; i < MAX_TIMER; i++) {
		if (timerctl.timers0[i].flags == 0) {
			timerctl.timers0[i].flags = TIMER_FLAGS_ALLOC;
			timerctl.timers0[i].flags2 = 0;
			return &timerctl.timers0[i];
		}
	}
	return 0; 
}

void timer_free(struct timer_struct *timer)
{
	timer->flags = 0; 
	return;
}

void timer_init(struct timer_struct *timer, struct pipe_struct *pipe, int data)
{
	timer->pipe = pipe;
	timer->data = data;
	return;
}

void timer_settime(struct timer_struct *timer, unsigned int timeout)
{
	int e;
	struct timer_struct *t, *s;
	timer->timeout = timeout + timerctl.count;
	timer->flags = TIMER_FLAGS_USING;
	e = io_load_eflags();
	io_cli();
	t = timerctl.t0;
	if (timer->timeout <= t->timeout) {
		
		timerctl.t0 = timer;
		timer->next = t; 
		timerctl.next = timer->timeout;
		io_store_eflags(e);
		return;
	}

	for (;;) {
		s = t;
		t = t->next;
		if (timer->timeout <= t->timeout) {
			
			s->next = timer;
			timer->next = t; 
			io_store_eflags(e);
			return;
		}
	}
}

void inthandler20(int *esp)
{
	struct timer_struct *timer;
	char ts = 0;
	io_out8(PIC0_OCW2, 0x60);	
	timerctl.count++;
	if (timerctl.next > timerctl.count) {
		return;
	}
	timer = timerctl.t0; 
	for (;;) {
		
		if (timer->timeout > timerctl.count) {
			break;
		}
		
		timer->flags = TIMER_FLAGS_ALLOC;
		if (timer != task_timer) {
			pipe_put(timer->pipe, timer->data);
		} else {
			ts = 1;
		}
		timer = timer->next;
	}
	timerctl.t0 = timer;
	timerctl.next = timer->timeout;
	if (ts != 0) {
		task_switch();
	}
	return;
}

int timer_cancel(struct timer_struct *timer)
{
	int e;
	struct timer_struct *t;
	e = io_load_eflags();
	io_cli();	
	if (timer->flags == TIMER_FLAGS_USING) {	
		if (timer == timerctl.t0) {
			
			t = timer->next;
			timerctl.t0 = t;
			timerctl.next = t->timeout;
		} else {
			
			t = timerctl.t0;
			for (;;) {
				if (t->next == timer) {
					break;
				}
				t = t->next;
			}
			t->next = timer->next; 
		}
		timer->flags = TIMER_FLAGS_ALLOC;
		io_store_eflags(e);
		return 1;	
	}
	io_store_eflags(e);
	return 0;
}

void timer_cancelall(struct pipe_struct *pipe)
{
	int e, i;
	struct timer_struct *t;
	e = io_load_eflags();
	io_cli();	
	for (i = 0; i < MAX_TIMER; i++) {
		t = &timerctl.timers0[i];
		if (t->flags != 0 && t->flags2 != 0 && t->pipe == pipe) {
			timer_cancel(t);
			timer_free(t);
		}
	}
	io_store_eflags(e);
	return;
}

