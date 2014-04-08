#include "slab.h"
#include "kmem.h"


//struct kmem_bufctl
void constr(void *buf, int size)
{
}
//struct kmem_bufctl
void destruct(void *buf, int size)
{
}



int slab_init()
{
	//test struct kmem_bufctl
	kmem_cache_t cache_test = kmem_cache_create("first", sizeof(struct kmem_bufctl), 0, &constr, &destruct);

	int count;
	void *items[100];
	for(count = 0; count < 100; count++){
		items[count] = kmem_cache_alloc(cache_test, KM_SLEEP);
	}

	for(count = 0; count < 100; count++){
		kmem_cache_free(cache_test, items[count]);
	}

	kmem_cache_reap(cache_test);
	kmem_cache_destroy(cache_test);
}

