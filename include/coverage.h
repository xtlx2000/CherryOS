#ifndef _COVERAGE_H_
#define _COVERAGE_H_

#define MAX_COVERAGES		256

struct coverage_struct {
	unsigned char *buf;
	int 		   bxsize, 
				   bysize, 
				   vx0, 
				   vy0, 
				   col_inv, 
				   height, 	/* level */
				   flags;	/* if it is used right now */
	struct coverage_controller *ctl;
	struct task_struct *task;
};

struct coverage_controller {
	unsigned char 	*vram, 	/* vram */
				  	*map; 	/* address the screen mem (xsize * ysize)*/
	int 			xsize, 	
					ysize, 	
					top;	/* top coverage code */
	struct coverage_struct *coverage_ptrs[MAX_COVERAGES];
	struct coverage_struct coverages[MAX_COVERAGES];
};

struct mem_controller;
struct coverage_controller *coverctl_init(struct mem_controller *memman, 
											unsigned char *vram, int xsize, int ysize);
struct coverage_struct *coverage_alloc(struct coverage_controller *ctl);
void coverage_setbuf(struct coverage_struct *sht, unsigned char *buf, 
											int xsize, int ysize, int col_inv);
void coverage_updown(struct coverage_struct *sht, int height);
void coverage_refresh(struct coverage_struct *sht, int bx0, int by0, int bx1, int by1);
void coverage_slide(struct coverage_struct *sht, int vx0, int vy0);
void coverage_free(struct coverage_struct *sht);


extern struct coverage_controller *shtctl;


#endif
