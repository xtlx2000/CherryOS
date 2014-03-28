#include "page.h"
#include "stdio.h"
#include "stdlib.h"

int paging_init(int detectsize)
{
	int number = detectsize / PAGESIZE + 1;
	mem_map = malloc(sizeof(struct page) * number);

	int i;
	for(i = 0; i < number; i++){
		struct page *tmp = &mem_map[i];
		//TO DO
	}
	
}
