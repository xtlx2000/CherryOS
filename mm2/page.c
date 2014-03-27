#include "page.h"

int paging_init(int detectsize)
{
	int number = detectsize / PAGESIZE + 1;
	mem_map = malloc(sizeof(struct page) * number);

	for(int i = 0; i < number; i++){
		struct page *tmp = mem_map[i];
		//TO DO
	}
	
}
