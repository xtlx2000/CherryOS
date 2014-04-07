#include "pm.h"
#include "page.h"
#include "stdio.h"
#include "stdlib.h"

int paging_init(int detectsize)
{
	int number = detectsize / PAGESIZE;
	mem_map = malloc(sizeof(struct page) * number);

	int i;
	for(i = 0; i < number; i++){
		struct page *page = &mem_map[i];
		page->location = (char *)memory_pointer + i*PAGESIZE;
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

