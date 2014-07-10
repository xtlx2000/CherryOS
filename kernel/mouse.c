#include "include/pipe.h"
#include "include/mouse.h"
#include "include/keyboard.h"
#include "include/int.h"
#include "include/interaction.h"


struct pipe_struct *mouse_pipe;
int mouse_data;

void inthandler2c(int *esp)
{
	int data;
	io_out8(PIC1_OCW2, 0x64);
	io_out8(PIC0_OCW2, 0x62);
	data = io_in8(PORT_KEYDAT);
	pipe_put(mouse_pipe, data + mouse_data);
	return;
}

#define KEYCMD_SENDTO_MOUSE		0xd4
#define MOUSECMD_ENABLE			0xf4

void enable_mouse(struct pipe_struct *pipe, int data, struct mouse_device *mdec)
{
	mouse_pipe = pipe;
	mouse_data = data;

	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);

	mdec->phase = 0; 
	return;
}

int mouse_decode(struct mouse_device *mdec, unsigned char dat)
{
	if (mdec->phase == 0) {
		
		if (dat == 0xfa) {
			mdec->phase = 1;
		}
		return 0;
	}
	if (mdec->phase == 1) {

		if ((dat & 0xc8) == 0x08) {
	
			mdec->buf[0] = dat;
			mdec->phase = 2;
		}
		return 0;
	}
	if (mdec->phase == 2) {

		mdec->buf[1] = dat;
		mdec->phase = 3;
		return 0;
	}
	if (mdec->phase == 3) {

		mdec->buf[2] = dat;
		mdec->phase = 1;
		mdec->btn = mdec->buf[0] & 0x07;
		mdec->x = mdec->buf[1];
		mdec->y = mdec->buf[2];
		if ((mdec->buf[0] & 0x10) != 0) {
			mdec->x |= 0xffffff00;
		}
		if ((mdec->buf[0] & 0x20) != 0) {
			mdec->y |= 0xffffff00;
		}
		mdec->y = - mdec->y; 
		return 1;
	}
	return -1;
}

