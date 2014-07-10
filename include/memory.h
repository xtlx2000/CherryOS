#ifndef _MEMORY_H_
#define _MEMORY_H_

#define MEMMAN_FREES		4090	
#define MEMMAN_ADDR			0x003c0000
struct free_segment {	
	unsigned int addr, size;
};
struct mem_controller {	
	int frees, 
		maxfrees, 
		lostsize, 
		losts;
	struct free_segment free[MEMMAN_FREES];
};

unsigned int memtest(unsigned int start, unsigned int end);
void memman_init(struct mem_controller *man);
unsigned int memman_total(struct mem_controller *man);
unsigned int memman_alloc(struct mem_controller *man, unsigned int size);
int memman_free(struct mem_controller *man, unsigned int addr, unsigned int size);
unsigned int memman_alloc_4k(struct mem_controller *man, unsigned int size);
int memman_free_4k(struct mem_controller *man, unsigned int addr, unsigned int size);


#endif
