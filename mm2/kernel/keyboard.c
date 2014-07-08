#include "include/keyboard.h"
#include "include/pipe.h"
#include "include/int.h"
#include "include/interaction.h"

struct pipe_struct keycmd;

struct pipe_struct *keyboard_pipe;
int keyboard_data;

void inthandler21(int *esp)
{
	int data;

	io_out8(PIC0_OCW2, 0x61);
	data = io_in8(PORT_KEYDAT);
	pipe_put(keyboard_pipe, data + keyboard_data);
	return;
}


#define PORT_KEYSTA				0x0064
#define KEYSTA_SEND_NOTREADY	0x02
#define KEYCMD_WRITE_MODE		0x60
#define KBC_MODE				0x47


void wait_KBC_sendready(void)
{
	for (;;) {
		if ((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0) {
			break;
		}
	}
	return;
}

void init_keyboard(struct pipe_struct *pipe, int data)
{

	keyboard_pipe = pipe;
	keyboard_data = data;

	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, KBC_MODE);
	return;
}

