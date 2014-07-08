
#include "include/pipe.h"
#include "include/sched.h"

#define FLAGS_OVERRUN		0x0001

void pipe_init(struct pipe_struct *pipe, int size, int *buf, struct task_struct *task)
{
	pipe->size = size;
	pipe->buf = buf;
	pipe->free = size; 
	pipe->flags = 0;
	pipe->p = 0; 
	pipe->q = 0; 
	pipe->task = task; 
	return;
}



int pipe_put(struct pipe_struct *pipe, int data)
{
	if (pipe->free == 0) {
		
		pipe->flags |= FLAGS_OVERRUN;
		return -1;
	}
	pipe->buf[pipe->p] = data;
	pipe->p++;
	if (pipe->p == pipe->size) {
		pipe->p = 0;
	}
	pipe->free--;
	if (pipe->task != 0) {
		if (pipe->task->flags != 2) { 
			task_run(pipe->task, -1, 0);
		}
	}
	
	return 0;
}



int pipe_get(struct pipe_struct *pipe)
{
	int data;
	if (pipe->free == pipe->size) {
		return -1;
	}
	data = pipe->buf[pipe->q];
	pipe->q++;
	if (pipe->q == pipe->size) {
		pipe->q = 0;
	}
	pipe->free++;
	return data;
}


int pipe_status(struct pipe_struct *pipe)
{
	return pipe->size - pipe->free;
}


