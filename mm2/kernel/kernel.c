#include "include/kernel.h"
#include "include/terminal.h"
#include "include/descriptor.h"
#include "include/pipe.h"
#include "include/keyboard.h"
#include "include/sched.h"
#include "include/mouse.h"
#include "include/file.h"
#include "include/timer.h"
#include "include/graphic.h"
#include "include/window.h"
#include "include/memory.h"
#include "include/int.h"
#include "include/coverage.h"
#include "include/interaction.h"

#include <stdio.h>

#define KEYCMD_LED		0xed


struct bootInfo *binfo;
struct pipe_struct pipe;


void keywin_off(struct coverage_struct *key_win);
void keywin_on(struct coverage_struct *key_win);

void HariMain(void)
{
	binfo = (struct bootInfo *) ADR_BOOTINFO;
	char s[40];
	
	int pipebuf[128], keycmd_buf[32], *cons_pipe[2];
	int mx, my, i;
	unsigned int memtotal;
	struct mouse_device mdec;
	struct mem_controller *memman = (struct mem_controller *) MEMMAN_ADDR;
	unsigned char *buf_back, buf_mouse[256], *buf_cons[2];
	struct coverage_struct *sht_back, *sht_mouse, *sht_cons[2];
	struct task_struct *task_a, *task_cons[2], *task;
	static char keytable0[0x80] = {
		0,   0,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '^', 0x08, 0,
		'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '@', '[', 0x0a, 0, 'A', 'S',
		'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', ':', 0,   0,   ']', 'Z', 'X', 'C', 'V',
		'B', 'N', 'M', ',', '.', '/', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
		'2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0x5c, 0,  0,   0,   0,   0,   0,   0,   0,   0,   0x5c, 0,  0
	};
	static char keytable1[0x80] = {
		0,   0,   '!', 0x22, '#', '$', '%', '&', 0x27, '(', ')', '~', '=', '~', 0x08, 0,
		'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '`', '{', 0x0a, 0, 'A', 'S',
		'D', 'F', 'G', 'H', 'J', 'K', 'L', '+', '*', 0,   0,   '}', 'Z', 'X', 'C', 'V',
		'B', 'N', 'M', '<', '>', '?', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
		'2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   '_', 0,   0,   0,   0,   0,   0,   0,   0,   0,   '|', 0,   0
	};
	int key_shift = 0, key_leds = (binfo->leds >> 4) & 7, keycmd_wait = -1;
	int j, x, y, mmx = -1, mmy = -1;
	struct coverage_struct *sht = 0, *key_win;

	init_gdtidt();
	init_pic();
	io_sti(); 
	pipe_init(&pipe, 128, pipebuf, 0);
	init_pit();
	init_keyboard(&pipe, 256);
	enable_mouse(&pipe, 512, &mdec);
	io_out8(PIC0_IMR, 0xf8); 
	io_out8(PIC1_IMR, 0xef); 
	pipe_init(&keycmd, 32, keycmd_buf, 0);

	/* 4MB-3GB mem check*/
	memtotal = memtest(0x00400000, 0xbfffffff);
	memman_init(memman);
	memman_free(memman, 0x00001000, 0x0009e000); 
	memman_free(memman, 0x00400000, memtotal - 0x00400000);

	init_palette();
	shtctl = coverctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);/* init shtctl */
	task_a = task_init(memman);/* task a*/
	pipe.task = task_a;
	task_run(task_a, 1, 2);
	*((int *) 0x0fe4) = (int) shtctl;

	/* sht_back   */
	sht_back  = coverage_alloc(shtctl);
	buf_back  = (unsigned char *) memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
	coverage_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1); 
	init_screen8(buf_back, binfo->scrnx, binfo->scrny);

	/* sht_cons */
	for (i = 0; i < 2; i++) {
		sht_cons[i] = coverage_alloc(shtctl);
		buf_cons[i] = (unsigned char *) memman_alloc_4k(memman, 356 * 365);
		
		coverage_setbuf(sht_cons[i], buf_cons[i], 356, 365, -1);
		make_window8(buf_cons[i], 356, 365, "terminal", 0);
		make_textbox8(sht_cons[i], 8, 28, 340, 328, COL8_000000);
		task_cons[i] = task_alloc();
		task_cons[i]->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024 - 12;
		task_cons[i]->tss.eip = (int) &terminal_task;
		task_cons[i]->tss.es = 1 * 8;
		task_cons[i]->tss.cs = 2 * 8;
		task_cons[i]->tss.ss = 1 * 8;
		task_cons[i]->tss.ds = 1 * 8;
		task_cons[i]->tss.fs = 1 * 8;
		task_cons[i]->tss.gs = 1 * 8;
		*((int *) (task_cons[i]->tss.esp + 4)) = (int) sht_cons[i];
		*((int *) (task_cons[i]->tss.esp + 8)) = memtotal;
		task_run(task_cons[i], 2, 2); /* level=2, priority=2 */
		sht_cons[i]->task = task_cons[i];
		sht_cons[i]->flags |= 0x20;	
		cons_pipe[i] = (int *) memman_alloc_4k(memman, 328 * 4);
		pipe_init(&task_cons[i]->pipe, 328, cons_pipe[i], task_cons[i]);
	}

	/* sht_mouse */
	sht_mouse = coverage_alloc(shtctl);
	coverage_setbuf(sht_mouse, buf_mouse, 16, 16, 99);
	init_mouse_cursor8(buf_mouse, 99);
	mx = (binfo->scrnx - 16) / 2; 
	my = (binfo->scrny - 28 - 16) / 2;

	//put coverages
	coverage_slide(sht_back,  0,  0);
	coverage_slide(sht_cons[1], 400,  6);
	coverage_slide(sht_cons[0],  8,  6);
	coverage_slide(sht_mouse, mx, my);
	//set height
	coverage_updown(sht_back,     0);
	coverage_updown(sht_cons[1],  1);
	coverage_updown(sht_cons[0],  2);
	coverage_updown(sht_mouse,    3);
	//set mouse
	key_win = sht_cons[0];
	keywin_on(key_win);

	pipe_put(&keycmd, KEYCMD_LED);
	pipe_put(&keycmd, key_leds);

	for (;;) {
		if (pipe_status(&keycmd) > 0 && keycmd_wait < 0) {
			
			keycmd_wait = pipe_get(&keycmd);
			wait_KBC_sendready();
			io_out8(PORT_KEYDAT, keycmd_wait);
		}
		io_cli();
		if (pipe_status(&pipe) == 0) {
			task_sleep(task_a);
			io_sti();
		} else {
			i = pipe_get(&pipe);
			io_sti();
			if (key_win->flags == 0) {	/* window close */
				key_win = shtctl->coverage_ptrs[shtctl->top - 1];
				keywin_on(key_win);
			}
			if (256 <= i && i <= 511) { /* keyboard data*/
				if (i < 0x80 + 256) { /* change to char */
					if (key_shift == 0) {
						s[0] = keytable0[i - 256];
					} else {
						s[0] = keytable1[i - 256];
					}
				} else {
					s[0] = 0;
				}
				//deal capsLocks
				if ('A' <= s[0] && s[0] <= 'Z') {	/* english char */
					if (((key_leds & 4) == 0 && key_shift == 0) ||
							((key_leds & 4) != 0 && key_shift != 0)) {
						s[0] += 0x20;	
					}
				}
				if (s[0] != 0) { 
					pipe_put(&key_win->task->pipe, s[0] + 256);
				}
				if (i == 256 + 0x0f) {	/* Tab */
					keywin_off(key_win);
					j = key_win->height - 1;
					if (j == 0) {
						j = shtctl->top - 1;
					}
					key_win = shtctl->coverage_ptrs[j];
					keywin_on(key_win);
				}
				if (i == 256 + 0x2a) {	/* left shift ON */
					key_shift |= 1;
				}
				if (i == 256 + 0x36) {	/* right shift ON */
					key_shift |= 2;
				}
				if (i == 256 + 0xaa) {	/* left shift OFF */
					key_shift &= ~1;
				}
				if (i == 256 + 0xb6) {	/* right shift OFF */
					key_shift &= ~2;
				}
				if (i == 256 + 0x3a) {	/* CapsLock */
					key_leds ^= 4;
					pipe_put(&keycmd, KEYCMD_LED);
					pipe_put(&keycmd, key_leds);
				}
				if (i == 256 + 0x45) {	/* NumLock */
					key_leds ^= 2;
					pipe_put(&keycmd, KEYCMD_LED);
					pipe_put(&keycmd, key_leds);
				}
				if (i == 256 + 0x46) {	/* ScrollLock */
					key_leds ^= 1;
					pipe_put(&keycmd, KEYCMD_LED);
					pipe_put(&keycmd, key_leds);
				}
				if (i == 256 + 0x3b && key_shift != 0) {
					task = key_win->task;
					if (task != 0 && task->tss.ss0 != 0) {	/* Shift+F1 */
						terminal_putstr0(task->cons, "\nBreak(key) :\n");
						io_cli();	
						task->tss.eax = (int) &(task->tss.esp0);
						task->tss.eip = (int) asm_end_app;
						io_sti();
					}
				}
				if (i == 256 + 0x57) {	/* F11 */
					coverage_updown(shtctl->coverage_ptrs[1], shtctl->top - 1);
				}
				if (i == 256 + 0xfa) {	
					keycmd_wait = -1;
				}
				if (i == 256 + 0xfe) {	
					wait_KBC_sendready();
					io_out8(PORT_KEYDAT, keycmd_wait);
				}
			} else if (512 <= i && i <= 767) { /* mouse data */
				if (mouse_decode(&mdec, i - 512) != 0) {
	
					mx += mdec.x;
					my += mdec.y;
					if (mx < 0) {
						mx = 0;
					}
					if (my < 0) {
						my = 0;
					}
					if (mx > binfo->scrnx - 1) {
						mx = binfo->scrnx - 1;
					}
					if (my > binfo->scrny - 1) {
						my = binfo->scrny - 1;
					}
					coverage_slide(sht_mouse, mx, my);
					if ((mdec.btn & 0x01) != 0) {
					
						if (mmx < 0) {
            
							for (j = shtctl->top - 1; j > 0; j--) {
								sht = shtctl->coverage_ptrs[j];
								x = mx - sht->vx0;
								y = my - sht->vy0;
								if (0 <= x && x < sht->bxsize && 0 <= y && y < sht->bysize) {
									if (sht->buf[y * sht->bxsize + x] != sht->col_inv) {
										coverage_updown(sht, shtctl->top - 1);
										if (sht != key_win) {
											keywin_off(key_win);
											key_win = sht;
											keywin_on(key_win);
										}
										if (3 <= x && x < sht->bxsize - 3 && 3 <= y && y < 21) {
											mmx = mx;	
											mmy = my;
										}
										if (sht->bxsize - 21 <= x && x < sht->bxsize - 5 && 5 <= y && y < 19) {
									
											if ((sht->flags & 0x10) != 0) {	
												task = sht->task;
												terminal_putstr0(task->cons, "\nBreak(mouse) :\n");
												io_cli();	
												task->tss.eax = (int) &(task->tss.esp0);
												task->tss.eip = (int) asm_end_app;
												io_sti();
											}
										}
										break;
									}
								}
							}
						} else {
							
							x = mx - mmx;	
							y = my - mmy;
							coverage_slide(sht, sht->vx0 + x, sht->vy0 + y);
							mmx = mx;
							mmy = my;
						}
					} else {
						
						mmx = -1;	
					}
				}
			}
		}
	}
}

void keywin_off(struct coverage_struct *key_win)
{
	change_wtitle8(key_win, 0);
	if ((key_win->flags & 0x20) != 0) {
		pipe_put(&key_win->task->pipe, 3);
	}
	return;
}

void keywin_on(struct coverage_struct *key_win)
{
	change_wtitle8(key_win, 1);
	if ((key_win->flags & 0x20) != 0) {
		pipe_put(&key_win->task->pipe, 2);
	}
	return;
}

