#include "include/coverage.h"
#include "include/memory.h"

#define COVERAGE_USE		1

struct coverage_controller *shtctl;


struct coverage_controller *coverctl_init(struct mem_controller *memman, unsigned char *vram, int xsize, int ysize)
{
	struct coverage_controller *ctl;
	int i;
	ctl = (struct coverage_controller *) memman_alloc_4k(memman, sizeof (struct coverage_controller));
	if (ctl == 0) {
		goto err;
	}
	ctl->map = (unsigned char *) memman_alloc_4k(memman, xsize * ysize);
	if (ctl->map == 0) {
		memman_free_4k(memman, (int) ctl, sizeof (struct coverage_controller));
		goto err;
	}
	ctl->vram = vram;
	ctl->xsize = xsize;
	ctl->ysize = ysize;
	ctl->top = -1; 
	for (i = 0; i < MAX_COVERAGES; i++) {
		ctl->coverages[i].flags = 0;  
		ctl->coverages[i].ctl = ctl; 
	}
err:
	return ctl;
}

struct coverage_struct *coverage_alloc(struct coverage_controller *ctl)
{
	struct coverage_struct *cover;
	int i;
	for (i = 0; i < MAX_COVERAGES; i++) {
		if (ctl->coverages[i].flags == 0) {
			cover = &ctl->coverages[i];
			cover->flags = COVERAGE_USE; 
			cover->height = -1; 
			cover->task = 0;	
			return cover;
		}
	}
	return 0;	
}

void coverage_setbuf(struct coverage_struct *cover, unsigned char *buf, int xsize, int ysize, int col_inv)
{
	cover->buf = buf;
	cover->bxsize = xsize;
	cover->bysize = ysize;
	cover->col_inv = col_inv;
	return;
}

void coverage_refreshmap(struct coverage_controller *ctl, int vx0, int vy0, int vx1, int vy1, int h0)
{
	int h, bx, by, vx, vy, bx0, by0, bx1, by1;
	unsigned char *buf, sid, *map = ctl->map;
	struct coverage_struct *cover;
	if (vx0 < 0) { vx0 = 0; }
	if (vy0 < 0) { vy0 = 0; }
	if (vx1 > ctl->xsize) { vx1 = ctl->xsize; }
	if (vy1 > ctl->ysize) { vy1 = ctl->ysize; }
	for (h = h0; h <= ctl->top; h++) {
		cover = ctl->coverage_ptrs[h];
		sid = cover - ctl->coverages;
		buf = cover->buf;
		bx0 = vx0 - cover->vx0;
		by0 = vy0 - cover->vy0;
		bx1 = vx1 - cover->vx0;
		by1 = vy1 - cover->vy0;
		if (bx0 < 0) { bx0 = 0; }
		if (by0 < 0) { by0 = 0; }
		if (bx1 > cover->bxsize) { bx1 = cover->bxsize; }
		if (by1 > cover->bysize) { by1 = cover->bysize; }
		for (by = by0; by < by1; by++) {
			vy = cover->vy0 + by;
			for (bx = bx0; bx < bx1; bx++) {
				vx = cover->vx0 + bx;
				if (buf[by * cover->bxsize + bx] != cover->col_inv) {
					map[vy * ctl->xsize + vx] = sid;
				}
			}
		}
	}
	return;
}

void coverage_refreshsub(struct coverage_controller *ctl, int vx0, int vy0, int vx1, int vy1, int h0, int h1)
{
	int h, bx, by, vx, vy, bx0, by0, bx1, by1;
	unsigned char *buf, *vram = ctl->vram, *map = ctl->map, sid;
	struct coverage_struct *cover;
	/* refresh adjust*/
	if (vx0 < 0) { vx0 = 0; }
	if (vy0 < 0) { vy0 = 0; }
	if (vx1 > ctl->xsize) { vx1 = ctl->xsize; }
	if (vy1 > ctl->ysize) { vy1 = ctl->ysize; }
	for (h = h0; h <= h1; h++) {
		cover = ctl->coverage_ptrs[h];
		buf = cover->buf;
		sid = cover - ctl->coverages;

		bx0 = vx0 - cover->vx0;
		by0 = vy0 - cover->vy0;
		bx1 = vx1 - cover->vx0;
		by1 = vy1 - cover->vy0;
		if (bx0 < 0) { bx0 = 0; }
		if (by0 < 0) { by0 = 0; }
		if (bx1 > cover->bxsize) { bx1 = cover->bxsize; }
		if (by1 > cover->bysize) { by1 = cover->bysize; }
		for (by = by0; by < by1; by++) {
			vy = cover->vy0 + by;
			for (bx = bx0; bx < bx1; bx++) {
				vx = cover->vx0 + bx;
				if (map[vy * ctl->xsize + vx] == sid) {
					vram[vy * ctl->xsize + vx] = buf[by * cover->bxsize + bx];
				}
			}
		}
	}
	return;
}

void coverage_updown(struct coverage_struct *cover, int height)
{
	struct coverage_controller *ctl = cover->ctl;
	int h, old = cover->height; /* save old height */

	/* adjust height */
	if (height > ctl->top + 1) {
		height = ctl->top + 1;
	}
	if (height < -1) {
		height = -1;
	}
	cover->height = height; 

	/* rearrange coverages */
	if (old > height) {	
		if (height >= 0) {
			
			for (h = old; h > height; h--) {
				ctl->coverage_ptrs[h] = ctl->coverage_ptrs[h - 1];
				ctl->coverage_ptrs[h]->height = h;
			}
			ctl->coverage_ptrs[height] = cover;
			coverage_refreshmap(ctl, cover->vx0, cover->vy0, cover->vx0 + cover->bxsize, cover->vy0 + cover->bysize, height + 1);
			coverage_refreshsub(ctl, cover->vx0, cover->vy0, cover->vx0 + cover->bxsize, cover->vy0 + cover->bysize, height + 1, old);
		} else {	
			if (ctl->top > old) {
				
				for (h = old; h < ctl->top; h++) {
					ctl->coverage_ptrs[h] = ctl->coverage_ptrs[h + 1];
					ctl->coverage_ptrs[h]->height = h;
				}
			}
			ctl->top--; 
			coverage_refreshmap(ctl, cover->vx0, cover->vy0, cover->vx0 + cover->bxsize, cover->vy0 + cover->bysize, 0);
			coverage_refreshsub(ctl, cover->vx0, cover->vy0, cover->vx0 + cover->bxsize, cover->vy0 + cover->bysize, 0, old - 1);/* 按新图层的信息重新绘制画面 */
		}
	} else if (old < height) {	
		if (old >= 0) {
			
			for (h = old; h < height; h++) {
				ctl->coverage_ptrs[h] = ctl->coverage_ptrs[h + 1];
				ctl->coverage_ptrs[h]->height = h;
			}
			ctl->coverage_ptrs[height] = cover;
		} else {	
			
			for (h = ctl->top; h >= height; h--) {
				ctl->coverage_ptrs[h + 1] = ctl->coverage_ptrs[h];
				ctl->coverage_ptrs[h + 1]->height = h + 1;
			}
			ctl->coverage_ptrs[height] = cover;
			ctl->top++;
		}
		coverage_refreshmap(ctl, cover->vx0, cover->vy0, cover->vx0 + cover->bxsize, cover->vy0 + cover->bysize, height);
		coverage_refreshsub(ctl, cover->vx0, cover->vy0, cover->vx0 + cover->bxsize, cover->vy0 + cover->bysize, height, height);//按新图层信息重新绘制画面
	}
	return;
}

void coverage_refresh(struct coverage_struct *cover, int bx0, int by0, int bx1, int by1)
{
	if (cover->height >= 0) {
		coverage_refreshsub(cover->ctl, cover->vx0 + bx0, cover->vy0 + by0, cover->vx0 + bx1, cover->vy0 + by1, cover->height, cover->height);
	}
	return;
}

void coverage_slide(struct coverage_struct *cover, int vx0, int vy0)
{
	struct coverage_controller *ctl = cover->ctl;
	int old_vx0 = cover->vx0, old_vy0 = cover->vy0;
	cover->vx0 = vx0;
	cover->vy0 = vy0;
	if (cover->height >= 0) { 
		coverage_refreshmap(ctl, old_vx0, old_vy0, old_vx0 + cover->bxsize, old_vy0 + cover->bysize, 0);
		coverage_refreshmap(ctl, vx0, vy0, vx0 + cover->bxsize, vy0 + cover->bysize, cover->height);
		coverage_refreshsub(ctl, old_vx0, old_vy0, old_vx0 + cover->bxsize, old_vy0 + cover->bysize, 0, cover->height - 1);
		coverage_refreshsub(ctl, vx0, vy0, vx0 + cover->bxsize, vy0 + cover->bysize, cover->height, cover->height);
	}
	return;
}

void coverage_free(struct coverage_struct *cover)
{
	if (cover->height >= 0) {
		coverage_updown(cover, -1); 
	}
	cover->flags = 0; 
	return;
}

