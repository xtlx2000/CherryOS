#include <stdio.h>
#include <stdlib.h>

#include "pm.h"
#include "pg_data.h"
#include "zone.h"
#include "buddy.h"
#include "slab.h"
#include "page.h"

#include "vm.h"



#include "fs.h"


#include "cache.h"
#include "buffer.h"

#include "macro.h"


int main()
{
	// 1 mm init
	memory_size = 20*1024*1024;//20MB
	memory_pointer = malloc(20*1024*1024);
	
	// 1.1 init page
	PRINT_DEBUG("page init:\n");
	paging_init(memory_size);
	
	
	// 1.2 init pg_data
	PRINT_DEBUG("\npg_data init:\n");
	//TODO: now all ram in pg[0].zone[normal]
	//long pg_data_size = memory_size / NR_CPUS;
	pg_data_init(memory_size);


	//1.3 test buddy alloc
	PRINT_DEBUG("\n================== Buddy Alloc Test ===========\n");
	int order = 0;
	struct page* pagesArr[MAX_ORDER];
	
	for(order = 0; order < MAX_ORDER; order++){
		pagesArr[order] = alloc_pages(order);
		if(pagesArr[order]){
			memset(pagesArr[order]->location, 0, (1<<order)*PAGESIZE);
		}
	}
	buddystatistics(&(pg[0].node_zones[ZONE_NORMAL]));

	// 1.3.2 test address page pfn convert
	PRINT_DEBUG("\n================== address page pfn convert Test ===========\n");

	long number = memory_size / PAGESIZE;
	int i;
	for(i = 0; i < number; i++){
		if(mem_map[i].private){
			PRINT_DEBUG("page %d private=%d\n", i, mem_map[i].private);
		}
	}


	
	void *addr = memory_pointer;
	PRINT_DEBUG("addr = %ld\n", (unsigned long)addr);
	PRINT_DEBUG("pfn = %ld\n", (unsigned long)__pa(addr));
	PRINT_DEBUG("page = %ld  private=%d\n", (unsigned long)virt_to_page(addr), virt_to_page(addr)->private);
	PRINT_DEBUG("pfn = %ld\n", page_to_pfn(virt_to_page(addr)));

	addr += 5*PAGESIZE;
	PRINT_DEBUG("addr = %ld\n", (unsigned long)addr);
	PRINT_DEBUG("pfn = %ld\n", (unsigned long)__pa(addr));
	PRINT_DEBUG("page = %ld  private=%d\n", (unsigned long)virt_to_page(addr), virt_to_page(addr)->private);
	PRINT_DEBUG("pfn = %ld\n", page_to_pfn(virt_to_page(addr)));

	addr = memory_pointer;
	for(i = 0; i < 5100; i++){
		
		if(virt_to_page(addr)->private){
			PRINT_DEBUG("page%d = %ld  private=%d\n",(unsigned long)__pa(addr), (unsigned long)virt_to_page(addr), virt_to_page(addr)->private);
		}

		addr += PAGESIZE;
	}
	

	
	
	

	//1.4 test buddy reclaim
	PRINT_DEBUG("\n================== Buddy Reclain Test ===========\n");
	PRINT_DEBUG("TODO\n");
	

	
	
	// 1.5 init slab
	PRINT_DEBUG("\n================== Slab Test ===========\n");
	slab_init();
	


	// 2 fs
	PRINT_DEBUG("\n================== FS Test ===========\n");
	fs_init();


	// 3 buffer cache
	PRINT_DEBUG("\n================== cache Test ===========\n");
	cache_init();

	
	return 0;
}
