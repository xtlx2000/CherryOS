#ifndef _MOUSE_H_
#define _MOUSE_H_

struct pipe_struct;
struct mouse_device;

struct mouse_device {
	unsigned char buf[3], phase;
	int x, y, btn;
};
void inthandler2c(int *esp);
void enable_mouse(struct pipe_struct *pipe, int data0, struct mouse_device *mdec);
int mouse_decode(struct mouse_device *mdec, unsigned char dat);



#endif
