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

struct page *pfn_to_page(unsigned long pfn)
{                                                                                                      
    return (struct page*)(memory_pointer + (pfn * PAGESIZE));
}

unsigned long page_to_pfn(struct page *page)
{
    return (page - (struct page*)memory_pointer);
}

