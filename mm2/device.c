#include "device.h"


int init_virtual_device(struct virtual_device *dev, void *address, u64 size)
{
	if(!dev || !address){
		return 1;
	}

	dev->address = address;
	dev->size = size;

	return 0;
}

