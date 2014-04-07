#include <stdio.h>
#include <stdlib.h>

#include "pm.h"
#include "pg_data.h"
#include "zone.h"
#include "buddy.h"
#include "slab.h"

#include "vm.h"



#include "vfs.h"
#include "ext2.h"


#include "cache.h"
#include "buffer.h"


int main()
{
	// 1 mm
	memory_size = 20 * 1024 * 1024;//20MB
	memory_pointer = malloc(20 * 1024 * 1024);
	
	// 1.1 init page
	paging_init();
	
	// 1.2 init pg_data
	long pg_data_size = memory_size / NR_CPUS;
	pg_data_init(pg_data_size);
	
	// 1.3 init buddy
	buddy_init();
	
	// 1.4 init slab
	slab_init();
	


	// 2 fs


	// 3 buffer cache

	
	return 0;
}
