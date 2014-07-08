#ifndef _KERNEL_H_
#define _KERNEL_H_

/* boot.nas */
//boot info
struct bootInfo { 	/* 0x0ff0-0x0fff */
	char cyls; 		/* boot area   */
	char leds; 		/* LED status  */
	char vmode; 	/* vmode  */
	char reserve;
	short scrnx, scrny; 
	char *vram;
};

#define ADR_BOOTINFO	0x00000ff0
#define ADR_DISKIMG		0x00100000

extern struct bootInfo *binfo;
extern struct pipe_struct pipe;


#endif

