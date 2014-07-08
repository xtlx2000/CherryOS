#ifndef _PIPE_H_
#define _PIPE_H_


struct pipe_struct {
	int *buf;
	int p, q, size, free, flags;
	struct task_struct *task;
};

void pipe_init(struct pipe_struct *pipe, int size, int *buf, struct task_struct *task);
int pipe_put(struct pipe_struct *pipe, int data);
int pipe_get(struct pipe_struct *pipe);
int pipe_status(struct pipe_struct *pipe);



#endif
