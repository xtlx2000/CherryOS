#include "fs.h"
#include "ext2.h"
#include "device.h"

int fs_init()
{
	// 1 vfs 
	vfs_init();
	// 2 real fs register
	init_ext2_fs();

	// 3 real device detect, we use sda
	void *addr = malloc(VIRTUAL_DEVICE_SIZE);
	int ret = 0;
	ret = init_virtual_device(&device_sda, addr, VIRTUAL_DEVICE_SIZE);
	if(ret != 0){
		PRINT_DEBUG("init_virtual_device error.\n");
		return ret;
	}

	// 4 format sda to ext2
	ret = format_to_ext2(&device_sda);

	// 5 mount sda
	
	
	return 0;
}
