#ifndef _COVERAGE_H_
#define _COVERAGE_H_

#define MAX_SHEETS		256
struct sheet_struct {
	unsigned char *buf;
	int bxsize, 
		bysize, 
		vx0, 
		vy0, 
		col_inv, 
		height, /* ���ڵĲ�� */
		flags;/* �Ƿ�ʹ�� */
	struct sheet_controller *ctl;
	struct task_struct *task;
};

struct sheet_controller {
	unsigned char *vram, /* vramΪ��Ӧ�ֱ��ʵ��Դ��ַ */
				  *map; /* ���ڴ���׼����������Ļ��UI����(xsize*ysize��С) */
	int xsize, /* ��Ӧ�ֱ��ʵĿ�� */
		ysize, /* ��Ӧ�ֱ��ʵĸ߶� */
		top;/* ���ϲ��sheet�ı��� */
	struct sheet_struct *sheets[MAX_SHEETS];
	struct sheet_struct sheets0[MAX_SHEETS];
};

struct mem_controller;
struct sheet_controller *shtctl_init(struct mem_controller *memman, unsigned char *vram, int xsize, int ysize);
struct sheet_struct *sheet_alloc(struct sheet_controller *ctl);
void sheet_setbuf(struct sheet_struct *sht, unsigned char *buf, int xsize, int ysize, int col_inv);
void sheet_updown(struct sheet_struct *sht, int height);
void sheet_refresh(struct sheet_struct *sht, int bx0, int by0, int bx1, int by1);
void sheet_slide(struct sheet_struct *sht, int vx0, int vy0);
void sheet_free(struct sheet_struct *sht);


#endif
