#include <stdio.h>
#include <stdlib.h>

#include "pm.h"
#include "pg_data.h"
#include "zone.h"
#include "buddy.h"
#include "slab.h"
#include "page.h"

#include "vm.h"



#include "vfs.h"
#include "ext2.h"


#include "cache.h"
#include "buffer.h"

#include "macro.h"


int main()
{
	// 1 mm init
	memory_size = 20*1024*1024;//20MB
	memory_pointer = malloc(20*1024*1024);
	
	// 1.1 init page
	DEBUG("page init:\n");
	paging_init(memory_size);
	
	// 1.2 init pg_data
	DEBUG("\npg_data init:\n");
	//TODO: now all ram in pg[0].zone[normal]
	//long pg_data_size = memory_size / NR_CPUS;
	pg_data_init(memory_size);


	//1.3 test buddy alloc
	DEBUG("\n================== Buddy Alloc Test ===========\n");
	int order = 0;
	struct page* pagesArr[MAX_ORDER];
	
	for(order = 0; order < MAX_ORDER; order++){
		pagesArr[order] = alloc_pages(order);
		if(pagesArr[order]){
			memset(pagesArr[order]->location, 0, (1<<order)*PAGESIZE);
		}
	}
	buddystatistics(&(pg[0].node_zones[ZONE_NORMAL]));

	//1.4 test buddy reclaim
	DEBUG("\n================== Buddy Reclain Test ===========\n");
	

	
	
	// 1.5 init slab
	slab_init();
	


	// 2 fs


	// 3 buffer cache

	
	return 0;
}
