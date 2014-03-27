#include <stdio.h>
#include <stdlib.h>

#include "pm.h"
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
	// 1.1 init page
	paging_init();
	// 1.2 init pg_data
	pg_data_init();
	// 1.3 init buddy
	
	// 1.4 init slab
	


	// 2 fs


	// 3 buffer cache

	
	return 0;
}
