#include "fs.h"


int fs_init()
{
	vfs_init();
	//ext2_register();
	return 0;
}
