#include "include/kernel.h"
#include "include/terminal.h"
#include "include/sched.h"
#include "include/timer.h"
#include "include/file.h"
#include "include/descriptor.h"
#include "include/graphic.h"
#include "include/memory.h"
#include "include/coverage.h"
#include "include/window.h"
#include "include/interaction.h"


#include <stdio.h>
#include <string.h>

void terminal_task(struct coverage_struct *cover, int memtotal)
{
	struct task_struct *task = task_now();
	struct mem_controller *memman = (struct mem_controller *) MEMMAN_ADDR;
	int i, *fat = (int *) memman_alloc_4k(memman, 4 * 2880);
	struct terminal cons;
	char cmdline[30];
	cons.sht = cover;
	cons.cur_x = 8;
	cons.cur_y = 28;
	cons.cur_c = -1;
	task->cons = &cons;

	cons.timer = timer_alloc();
	timer_init(cons.timer, &task->pipe, 1);
	timer_settime(cons.timer, 50);
	file_readfat(fat, (unsigned char *) (ADR_DISKIMG + 0x000200));
	
	terminal_putstr1(&cons, "Welcome to CherryOS", 19);
	terminal_newline(&cons);
	terminal_putstr1(&cons, "http://lvpengcheng.com", 22);
	

	terminal_newline(&cons);
	terminal_putchar(&cons, '>', 1);

	for (;;) {
		io_cli();
		if (pipe_status(&task->pipe) == 0) {
			task_sleep(task);
			io_sti();
		} else {
			i = pipe_get(&task->pipe);
			io_sti();
			if (i <= 1) {
				if (i != 0) {
					timer_init(cons.timer, &task->pipe, 0);
					if (cons.cur_c >= 0) {
						cons.cur_c = COL8_FFFFFF;
					}
				} else {
					timer_init(cons.timer, &task->pipe, 1); 
					if (cons.cur_c >= 0) {
						cons.cur_c = COL8_000000;
					}
				}
				timer_settime(cons.timer, 50);
			}
			if (i == 2) {	
				cons.cur_c = COL8_FFFFFF;
			}
			if (i == 3) {	
				boxfill8(cover->buf, cover->bxsize, COL8_000000, cons.cur_x, cons.cur_y, cons.cur_x + 7, cons.cur_y + 15);
				cons.cur_c = -1;
			}
			if (256 <= i && i <= 511) { 
				if (i == 8 + 256) {

					if (cons.cur_x > 16) {
			
						terminal_putchar(&cons, ' ', 0);
						cons.cur_x -= 8;
					}
				} else if (i == 10 + 256) {
					/* Enter */

					terminal_putchar(&cons, ' ', 0);
					cmdline[cons.cur_x / 8 - 2] = 0;
					terminal_newline(&cons);
					terminal_runcmd(cmdline, &cons, fat, memtotal);

					terminal_putchar(&cons, '>', 1);
				} else {
					/* normal */
					if (cons.cur_x < 340) {
						
						cmdline[cons.cur_x / 8 - 2] = i - 256;
						terminal_putchar(&cons, i - 256, 1);
					}
				}
			}
			
			if (cons.cur_c >= 0) {
				boxfill8(cover->buf, cover->bxsize, cons.cur_c, cons.cur_x, cons.cur_y, cons.cur_x + 7, cons.cur_y + 15);
			}
			coverage_refresh(cover, cons.cur_x, cons.cur_y, cons.cur_x + 8, cons.cur_y + 16);
		}
	}
}

void terminal_putchar(struct terminal *cons, int chr, char move)
{
	char s[2];
	s[0] = chr;
	s[1] = 0;
	if (s[0] == 0x09) {
		for (;;) {
			putfonts8_asc_sht(cons->sht, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, " ", 1);
			cons->cur_x += 8;
			if (cons->cur_x == 8 + 240) {
				terminal_newline(cons);
			}
			if (((cons->cur_x - 8) & 0x1f) == 0) {
				break;	
			}
		}
	} else if (s[0] == 0x0a) {	
		terminal_newline(cons);
	} else if (s[0] == 0x0d) {	
		//nothing
	} else {
		putfonts8_asc_sht(cons->sht, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, s, 1);
		if (move != 0) {
			
			cons->cur_x += 8;
			if (cons->cur_x == 8 + 240) {
				terminal_newline(cons);
			}
		}
	}
	return;
}

void terminal_newline(struct terminal *cons)
{
	int x, y;
	struct coverage_struct *cover = cons->sht;
	if (cons->cur_y < 28 + 14*16) {
		cons->cur_y += 16; 
	} else {
		
		for (y = 28; y < 28 + 14*16; y++) {
			for (x = 8; x < 8 + 340; x++) {
				cover->buf[x + y * cover->bxsize] = cover->buf[x + (y + 16) * cover->bxsize];
			}
		}
		for (y = 28 + 14*16; y < 28 + 15*16; y++) {
			for (x = 8; x < 8 + 340; x++) {
				cover->buf[x + y * cover->bxsize] = COL8_000000;
			}
		}
		coverage_refresh(cover, 8, 28, 8 + 340, 28 + 15*16);
	}
	cons->cur_x = 8;
	return;
}

void terminal_putstr0(struct terminal *cons, char *s)
{
	for (; *s != 0; s++) {
		terminal_putchar(cons, *s, 1);
	}
	return;
}

void terminal_putstr1(struct terminal *cons, char *s, int l)
{
	int i;
	for (i = 0; i < l; i++) {
		terminal_putchar(cons, s[i], 1);
	}
	return;
}

void terminal_runcmd(char *cmdline, struct terminal *cons, int *fat, int memtotal)
{
	if (strcmp(cmdline, "mem") == 0) {
		cmd_mem(cons, memtotal);
	} else if (strcmp(cmdline, "clear") == 0) {
		cmd_cls(cons);
	} else if (strcmp(cmdline, "ls") == 0) {
		cmd_dir(cons);
	} else if (strncmp(cmdline, "cat ", 4) == 0) {
		cmd_type(cons, fat, cmdline);
	} else if (cmdline[0] != 0) {
		if (cmd_app(cons, fat, cmdline) == 0) {

			terminal_putstr0(cons, "bash: command not found.\n\n");
		}
	}
	return;
}

void cmd_mem(struct terminal *cons, int memtotal)
{
	struct mem_controller *memman = (struct mem_controller *) MEMMAN_ADDR;
	char s[60];
	sprintf(s, "total   %dMB\nfree %dKB\n\n", memtotal / (1024 * 1024), memman_total(memman) / 1024);
	terminal_putstr0(cons, s);
	return;
}

void cmd_cls(struct terminal *cons)
{
	int x, y;
	struct coverage_struct *cover = cons->sht;
	for (y = 28; y < 28 + 15*16; y++) {
		for (x = 8; x < 8 + 340; x++) {
			cover->buf[x + y * cover->bxsize] = COL8_000000;
		}
	}
	coverage_refresh(cover, 8, 28, 8 + 340, 28 + 15*16);
	cons->cur_y = 28;
	return;
}

void cmd_dir(struct terminal *cons)
{
	struct file_struct *finfo = (struct file_struct *) (ADR_DISKIMG + 0x002600);
	int i, j;
	char s[30];
	for (i = 0; i < 224; i++) {
		if (finfo[i].name[0] == 0x00) {
			break;
		}
		if (finfo[i].name[0] != 0xe5) {
			if ((finfo[i].type & 0x18) == 0) {
				sprintf(s, "filename.ext   %7d\n", finfo[i].size);
				for (j = 0; j < 8; j++) {
					s[j] = finfo[i].name[j];
				}
				s[ 9] = finfo[i].ext[0];
				s[10] = finfo[i].ext[1];
				s[11] = finfo[i].ext[2];
				terminal_putstr0(cons, s);
			}
		}
	}
	terminal_newline(cons);
	return;
}

void cmd_type(struct terminal *cons, int *fat, char *cmdline)
{
	struct mem_controller *memman = (struct mem_controller *) MEMMAN_ADDR;
	struct file_struct *finfo = file_search(cmdline + 4, (struct file_struct *) (ADR_DISKIMG + 0x002600), 224);
	char *p;
	if (finfo != 0) {
	
		p = (char *) memman_alloc_4k(memman, finfo->size);
		file_loadfile(finfo->clustno, finfo->size, p, fat, (char *) (ADR_DISKIMG + 0x003e00));
		terminal_putstr1(cons, p, finfo->size);
		memman_free_4k(memman, (int) p, finfo->size);
	} else {
		
		terminal_putstr0(cons, "File not found.\n");
	}
	terminal_newline(cons);
	return;
}

int cmd_app(struct terminal *cons, int *fat, char *cmdline)
{
	struct mem_controller *memman = (struct mem_controller *) MEMMAN_ADDR;
	struct file_struct *finfo;
	struct segment_descriptor *gdt = (struct segment_descriptor *) ADR_GDT;
	char name[18], *p, *q;
	struct task_struct *task = task_now();
	int i, segsiz, datsiz, esp, dathrb;
	struct coverage_controller *shtctl;
	struct coverage_struct *sht;


	for (i = 0; i < 13; i++) {
		if (cmdline[i] <= ' ') {
			break;
		}
		name[i] = cmdline[i];
	}
	name[i] = 0; 


	finfo = file_search(name, (struct file_struct *) (ADR_DISKIMG + 0x002600), 224);
	if (finfo == 0 && name[i - 1] != '.') {
		
		name[i    ] = '.';
		name[i + 1] = 'H';
		name[i + 2] = 'R';
		name[i + 3] = 'B';
		name[i + 4] = 0;
		finfo = file_search(name, (struct file_struct *) (ADR_DISKIMG + 0x002600), 224);
	}

	if (finfo != 0) {
		
		p = (char *) memman_alloc_4k(memman, finfo->size);
		file_loadfile(finfo->clustno, finfo->size, p, fat, (char *) (ADR_DISKIMG + 0x003e00));
		if (finfo->size >= 36 && strncmp(p + 4, "Hari", 4) == 0 && *p == 0x00) {
			segsiz = *((int *) (p + 0x0000));
			esp    = *((int *) (p + 0x000c));
			datsiz = *((int *) (p + 0x0010));
			dathrb = *((int *) (p + 0x0014));
			q = (char *) memman_alloc_4k(memman, segsiz);
			task->ds_base = (int) q;
			set_segmdesc(gdt + task->sel / 8 + 1000, finfo->size - 1, (int) p, AR_CODE32_ER + 0x60);
			set_segmdesc(gdt + task->sel / 8 + 2000, segsiz - 1,      (int) q, AR_DATA32_RW + 0x60);
			for (i = 0; i < datsiz; i++) {
				q[esp + i] = p[dathrb + i];
			}
			start_app(0x1b, task->sel + 1000 * 8, esp, task->sel + 2000 * 8, &(task->tss.esp0));
			shtctl = (struct coverage_controller *) *((int *) 0x0fe4);
			for (i = 0; i < MAX_COVERAGES; i++) {
				sht = &(shtctl->coverages[i]);
				if ((sht->flags & 0x11) == 0x11 && sht->task == task) {
				
					coverage_free(sht);	
				}
			}
			timer_cancelall(&task->pipe);
			memman_free_4k(memman, (int) q, segsiz);
		} else {
			terminal_putstr0(cons, ".hrb file format error.\n");
		}
		memman_free_4k(memman, (int) p, finfo->size);
		terminal_newline(cons);
		return 1;
	}

	return 0;
}


int *hrb_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx, int eax)
{
	struct task_struct *task = task_now();
	int ds_base = task->ds_base;
	struct terminal *cons = task->cons;
	struct coverage_controller *shtctl = (struct coverage_controller *) *((int *) 0x0fe4);
	struct coverage_struct *sht;
	int *reg = &eax + 1;	

	int i;

	if (edx == 1) {
		terminal_putchar(cons, eax & 0xff, 1);
	} else if (edx == 2) {
		terminal_putstr0(cons, (char *) ebx + ds_base);
	} else if (edx == 3) {
		terminal_putstr1(cons, (char *) ebx + ds_base, ecx);
	} else if (edx == 4) {
		return &(task->tss.esp0);
	} else if (edx == 5) {
		sht = coverage_alloc(shtctl);
		sht->task = task;
		sht->flags |= 0x10;
		coverage_setbuf(sht, (char *) ebx + ds_base, esi, edi, eax);
		make_window8((char *) ebx + ds_base, esi, edi, (char *) ecx + ds_base, 0);
		coverage_slide(sht, (shtctl->xsize - esi) / 2, (shtctl->ysize - edi) / 2);
		coverage_updown(sht, shtctl->top);
		reg[7] = (int) sht;
	} else if (edx == 6) {
		sht = (struct coverage_struct *) (ebx & 0xfffffffe);
		putfonts8_asc(sht->buf, sht->bxsize, esi, edi, eax, (char *) ebp + ds_base);
		if ((ebx & 1) == 0) {
			coverage_refresh(sht, esi, edi, esi + ecx * 8, edi + 16);
		}
	} else if (edx == 7) {
		sht = (struct coverage_struct *) (ebx & 0xfffffffe);
		boxfill8(sht->buf, sht->bxsize, ebp, eax, ecx, esi, edi);
		if ((ebx & 1) == 0) {
			coverage_refresh(sht, eax, ecx, esi + 1, edi + 1);
		}
	} else if (edx == 8) {
		memman_init((struct mem_controller *) (ebx + ds_base));
		ecx &= 0xfffffff0;	
		memman_free((struct mem_controller *) (ebx + ds_base), eax, ecx);
	} else if (edx == 9) {
		ecx = (ecx + 0x0f) & 0xfffffff0; 
		reg[7] = memman_alloc((struct mem_controller *) (ebx + ds_base), ecx);
	} else if (edx == 10) {
		ecx = (ecx + 0x0f) & 0xfffffff0;
		memman_free((struct mem_controller *) (ebx + ds_base), eax, ecx);
	} else if (edx == 11) {
		sht = (struct coverage_struct *) (ebx & 0xfffffffe);
		sht->buf[sht->bxsize * edi + esi] = eax;
		if ((ebx & 1) == 0) {
			coverage_refresh(sht, esi, edi, esi + 1, edi + 1);
		}
	} else if (edx == 12) {
		sht = (struct coverage_struct *) ebx;
		coverage_refresh(sht, eax, ecx, esi, edi);
	} else if (edx == 13) {
		sht = (struct coverage_struct *) (ebx & 0xfffffffe);
		hrb_api_linewin(sht, eax, ecx, esi, edi, ebp);
		if ((ebx & 1) == 0) {
			coverage_refresh(sht, eax, ecx, esi + 1, edi + 1);
		}
	} else if (edx == 14) {
		coverage_free((struct coverage_struct *) ebx);
	} else if (edx == 15) {
		for (;;) {
			io_cli();
			if (pipe_status(&task->pipe) == 0) {
				if (eax != 0) {
					task_sleep(task);	
				} else {
					io_sti();
					reg[7] = -1;
					return 0;
				}
			}
			i = pipe_get(&task->pipe);
			io_sti();
			if (i <= 1) { 
				
				timer_init(cons->timer, &task->pipe, 1); 
				timer_settime(cons->timer, 50);
			}
			if (i == 2) {	
				cons->cur_c = COL8_FFFFFF;
			}
			if (i == 3) {	
				cons->cur_c = -1;
			}
			if (i >= 256) {
				reg[7] = i - 256;
				return 0;
			}
		}
	} else if (edx == 16) {
		reg[7] = (int) timer_alloc();
		((struct timer_struct *) reg[7])->flags2 = 1;	
	} else if (edx == 17) {
		timer_init((struct timer_struct *) ebx, &task->pipe, eax + 256);
	} else if (edx == 18) {
		timer_settime((struct timer_struct *) ebx, eax);
	} else if (edx == 19) {
		timer_free((struct timer_struct *) ebx);
	} else if (edx == 20) {
		if (eax == 0) {
			i = io_in8(0x61);
			io_out8(0x61, i & 0x0d);
		} else {
			i = 1193180000 / eax;
			io_out8(0x43, 0xb6);
			io_out8(0x42, i & 0xff);
			io_out8(0x42, i >> 8);
			i = io_in8(0x61);
			io_out8(0x61, (i | 0x03) & 0x0f);
		}
	}
	return 0;
}

int *inthandler0c(int *esp)
{
	struct task_struct *task = task_now();
	struct terminal *cons = task->cons;
	char s[30];
	terminal_putstr0(cons, "\nINT 0C :\n Stack Exception.\n");
	sprintf(s, "EIP = %08X\n", esp[11]);
	terminal_putstr0(cons, s);
	return &(task->tss.esp0);	
}

int *inthandler0d(int *esp)
{
	struct task_struct *task = task_now();
	struct terminal *cons = task->cons;
	char s[30];
	terminal_putstr0(cons, "\nINT 0D :\n General Protected Exception.\n");
	sprintf(s, "EIP = %08X\n", esp[11]);
	terminal_putstr0(cons, s);
	return &(task->tss.esp0);	
}

void hrb_api_linewin(struct coverage_struct *sht, int x0, int y0, int x1, int y1, int col)
{
	int i, x, y, len, dx, dy;

	dx = x1 - x0;
	dy = y1 - y0;
	x = x0 << 10;
	y = y0 << 10;
	if (dx < 0) {
		dx = - dx;
	}
	if (dy < 0) {
		dy = - dy;
	}
	if (dx >= dy) {
		len = dx + 1;
		if (x0 > x1) {
			dx = -1024;
		} else {
			dx =  1024;
		}
		if (y0 <= y1) {
			dy = ((y1 - y0 + 1) << 10) / len;
		} else {
			dy = ((y1 - y0 - 1) << 10) / len;
		}
	} else {
		len = dy + 1;
		if (y0 > y1) {
			dy = -1024;
		} else {
			dy =  1024;
		}
		if (x0 <= x1) {
			dx = ((x1 - x0 + 1) << 10) / len;
		} else {
			dx = ((x1 - x0 - 1) << 10) / len;
		}
	}

	for (i = 0; i < len; i++) {
		sht->buf[(y >> 10) * sht->bxsize + (x >> 10)] = col;
		x += dx;
		y += dy;
	}

	return;
}

