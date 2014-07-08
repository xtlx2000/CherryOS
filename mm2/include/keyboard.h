#ifndef _KEYBOARD_H_
#define _KEYBOARD_H_

#include "pipe.h"

void inthandler21(int *esp);
void wait_KBC_sendready(void);
void init_keyboard(struct pipe_struct *pipe, int data0);

#define PORT_KEYDAT		0x0060
#define PORT_KEYCMD		0x0064

extern struct pipe_struct keycmd;


#endif
