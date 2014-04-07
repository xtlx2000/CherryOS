#include "pm.h"
#include "page.h"
#include "stdio.h"
#include "stdlib.h"

#include "macro.h"

int paging_init(long detectsize)
{
	long number = detectsize / PAGESIZE;
	DEBUG("page number = %d\n", number);
	mem_map = malloc(sizeof(struct page) * number);

	long i;
	for(i = 0; i < number; i++){
		struct page *page = &mem_map[i];
		page->location = (char *)memory_pointer + i*PAGESIZE;
		
		DEBUG("  page %d:  0x%ld\n", i, page->location);
	}

	return 0;
	
}

struct page *pfn_to_page(unsigned long pfn)
{                                                                                                      
    return &mem_map[pfn];
}

unsigned long page_to_pfn(struct page *page)
{
    return (page - (struct page*)mem_map);
}

struct page *pfn_to_page2(unsigned long pfn)
{
	unsigned long index = pfn - (unsigned long)memory_pointer;
    return &mem_map[pfn];
}

unsigned long page_to_pfn2(struct page *page)
{
    unsigned long index = (page - (struct page*)mem_map);
	unsigned long addr = (unsigned long)memory_pointer + (index*PAGESIZE);

	return addr >> PAGE_SHIFT;
}


