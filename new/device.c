#include "device.h"
#include "string.h"


int device_init()
{
	//sda
	void *addr = malloc(VIRTUAL_DEVICE_SIZE);
	int ret = 0;
	ret = init_virtual_device(&device_sda, "sda", addr, VIRTUAL_DEVICE_SIZE);
	if(ret != 0){
		PRINT_DEBUG("init_virtual_device error.\n");
		return ret;
	}

	list_add(&device_list, &device_ada.dev_list);

	
	
}




int init_virtual_device(struct virtual_device *dev, char *name, void *address, u64 size)
{
	if(!name || !dev || !address){
		return -1;
	}

	memset(dev->name, 0, MAX_DEVICE_NAMELEN);
	memcpy(dev->name, name);
	dev->address = address;
	dev->size = size;

	return 0;
}

