#include "pm.h"
#include "zone.h"
#include "list.h"
#include "page.h"
#include "pg_data.h"

int per_cpu_pages_init(struct per_cpu_pageset *pageset);
int zone_init_free_lists(struct zone *z);
struct page *alloc_pages_current(unsigned order);
struct page *__alloc_pages_node(unsigned int order, struct zonelist *zonelist);
static struct page *get_page_from_freelist(unsigned int order,struct zonelist *zonelist);
struct page *buffered_rmqueue(struct zone *zone, int order);
struct page *__alloc_pages_slowpath(unsigned int order,struct zonelist *zonelist);
struct page *__rmqueue_smallest(struct zone *zone, unsigned int order);
void expand(struct zone *zone, struct page *page, int low, int high, struct free_area *area);
int rmqueue_bulk(struct zone *zone, unsigned int order, unsigned long count, struct list_head *list);
struct page *__rmqueue(struct zone *zone, unsigned int order);
struct page *__alloc_pages_direct_reclaim(unsigned int order);
void __free_pages(struct page *page, unsigned int order);
int put_page_testzero(struct page *page);
void free_hot_page(struct page *page);
void free_hot_cold_page(struct page *page, int cold);
void free_pages_bulk(struct zone *zone, int count,
                    struct list_head *list, int order);
void __free_one_page(struct page *page,
        struct zone *zone, unsigned int order);
struct page *__page_find_buddy(struct page *page, unsigned long page_idx, unsigned int order);
int page_is_buddy(struct page *page, struct page *buddy, int order);
unsigned long page_order(struct page *page);
unsigned long __find_combined_index(unsigned long page_idx, unsigned int order);
void __free_pages_ok(struct page *page, unsigned int order);













#define zone_pcp(__z, __cpu) ((__z)->pageset[(__cpu)])




int zone_init(struct zone *z, long pfn, long memory_size)
{
	z->pages_min = 0;
	z->pages_low = 0;
	z->pages_high = 0;

	z->lowmem_reserve[ZONE_NORMAL] = 0;
	z->lowmem_reserve[ZONE_HIGHMEM] = 0;
	z->lowmem_reserve[ZONE_MOVABLE] = 0;

	z->min_unmapped_pages = 0;
	z->min_slab_pages = 0;

	int i;
	for(i = 0; i < NR_CPUS; i++){
		per_cpu_pages_init(&(z->pageset[i]));
	}

	//init the buddy
	zone_init_free_lists(z);

	//add ram to buddy
	long left = memory_size;
	long leftpage = memory_size / PAGESIZE;

	while(leftpage > 0){

		int order = MAX_ORDER-1;
		while(order){

			if((1 << order) > leftpage){
				order--;
				continue;
			}

			//add pfn to the order list
			//struct page *page = NULL;
			//list_for_each_entry(page, &(z->free_area[order].free_list[0]), lru){
			//	if(page->location)
			//}
			list_add(&(pfn_to_page(pfn)->lru), &(z->free_area[order].free_list[0]));
			set_page_private(pfn_to_page(pfn), order);
			
			leftpage -= (1 << order);
			pfn += (1 << order);
			z->free_area[order].nr_free++;
		}

		
	}

	char *name = (char *)malloc(sizeof("name"));
	memset(name, "name", sizeof("name"));
	z->name = name;
	

	//statistics and print
	buddystatistics(z);
	



	
	return 0;
}

int buddystatistics(struct zone *z)
{
	int total_pages = 0;
	int i;
	for(i = 0; i < MAX_ORDER; i++){
		total_pages += z->free_area[i].nr_free * (1 << i);
		DEBUG("      order %d have %d objects(%d pages).\n", i, z->free_area[i].nr_free, z->free_area[i].nr_free * (1 << i));
	}
	DEBUG("      Total have %d pages.\n", total_pages);

}



#define for_each_migratetype_order(order, type) \
	for (order = 0; order < MAX_ORDER; order++) \
		for (type = 0; type < MIGRATE_TYPES; type++)


int zone_init_free_lists(struct zone *z)
{
	int order, t;
	for_each_migratetype_order(order, t) {
		INIT_LIST_HEAD(&z->free_area[order].free_list[t]);  //初始化链表
		z->free_area[order].nr_free = 0;  //初始化nr_free为0
	}
}

int per_cpu_pages_init(struct per_cpu_pageset *pageset)
{
	pageset->pcp.count = 0;		/* number of pages in the list */
	pageset->pcp.high = 0;		/* high watermark, emptying needed */
	pageset->pcp.batch = PCPBATCH;		/* chunk size for buddy add/remove */
	INIT_LIST_HEAD(&pageset->pcp.list);	/* the list of pages */

	return 0;
}








struct page *
alloc_pages(unsigned int order)
{    
    if(order >= MAX_ORDER)
        return 0;

    return alloc_pages_current(order);
}


/**
 *  alloc_pages_current - Allocate pages.
 *
 *  @gfp:
 *      %GFP_USER   user allocation,
 *          %GFP_KERNEL kernel allocation,
 *          %GFP_HIGHMEM highmem allocation,
 *          %GFP_FS     don't call back into a file system.
 *          %GFP_ATOMIC don't sleep.
 *  @order: Power of two of allocation size in pages. 0 is a single page.
 *
 *  Allocate a page from the kernel page pool.  When not in
 *  interrupt context and apply the current process NUMA policy.
 *  Returns NULL when no page can be allocated.
 *
 *  Don't call cpuset_update_task_memory_state() unless
 *  1) it's ok to take cpuset_sem (can WAIT), and
 *  2) allocating for current task (not interrupt).
 */
struct page *alloc_pages_current(unsigned int  order)                                                 
{
	//TODO: current
    pg_data_t* pg_data = &pg[0];

    return __alloc_pages_node(order, &(pg[0].node_zonelists[0]));
    
}


/*
 * This is the 'heart' of the zoned buddy allocator.
 */
struct page *  
__alloc_pages_node(unsigned int order, struct zonelist *zonelist)
{
	struct page *page;
	/* health check */
	//TODO
	
	/* First allocation attempt : alloc from pcp */
	page = get_page_from_freelist(order, zonelist);

	/* slow allocation */
	if(!page){
		
		page = __alloc_pages_slowpath(order, zonelist);
	}

	return page;
}


struct page *
get_page_from_freelist(unsigned int order, struct zonelist *zonelist)
{
	struct page *page;
	page = buffered_rmqueue(&(pg[0].node_zones[ZONE_NORMAL]), order);

	return page;
}


/*
 * Really, prep_compound_page() should be called from __rmqueue_bulk().  But
 * we cheat by calling it from here, in the order > 0 path.  Saves a branch
 * or two.
 */
struct page *buffered_rmqueue(struct zone *zone, int order)
{
    unsigned long flags;
    struct page *page;
    int cpu = 0;

again:
    //cpu  = get_cpu();
    if (likely(order == 0)) {
        struct per_cpu_pages *pcp;

        pcp = &zone_pcp(zone, cpu).pcp;
        if (!pcp->count) {
            pcp->count = rmqueue_bulk(zone, 0,
                    pcp->batch, &pcp->list);
            if (unlikely(!pcp->count))
                goto failed;
        }

        if(pcp->count){
        	page = list_first_entry(&pcp->list, struct page, lru);
        	list_del(&page->lru);
        	pcp->count--;
        }
        
    } else {
    
        //spin_lock_irqsave(&zone->lock, flags);
        page = __rmqueue(zone, order);
        //spin_unlock(&zone->lock);                            
        if (!page)
            goto failed;
    }

    //__count_zone_vm_events( zone, 1 << order);
    //zone_statistics(zone);
    //local_irq_restore(flags);
    //put_cpu();

    //VM_BUG_ON(bad_range(zone, page));
    //if (prep_new_page(page, order, gfp_flags))
    //    goto again;
    return page;

failed:
    //local_irq_restore(flags);
    //put_cpu();
    return 0;
}

/*  
 * Obtain a specified number of elements from the buddy allocator, all under
 * a single hold of the lock, for efficiency.  Add them to the supplied list.
 * Returns the number of new pages which were placed at *list.
 */
int rmqueue_bulk(struct zone *zone, unsigned int order, 
            unsigned long count, struct list_head *list)
{
    int i;
    
    //spin_lock(&zone->lock);
    for (i = 0; i < count; ++i) {
        struct page *page = __rmqueue(zone, order);
        if (unlikely(page == 0))
            break;

    /*
         * Split buddy pages returned by expand() are received here
         * in physical page order. The page is added to the callers and
         * list and the list head then moves forward. From the callers
         * perspective, the linked list is ordered by page number in
         * some conditions. This is useful for IO devices that can
         * merge IO requests if the physical pages are ordered
         * properly.
         */
        list_add(&page->lru, list);
        set_page_private(page, order);
        list = &page->lru;
    }
    //spin_unlock(&zone->lock);
    return i;
} 


/*
 * Do the hard work of removing an element from the buddy allocator.                                        
 * Call me with the zone->lock already held.
 */
struct page *__rmqueue(struct zone *zone, unsigned int order)
{
    struct page *page;

    page = __rmqueue_smallest(zone, order);
    
    return page;
}


/* Remove an element from the buddy allocator from the fallback list */
/*
static struct page *__rmqueue_fallback(struct zone *zone, int order,
                        int start_migratetype)
{
    struct free_area * area;
    int current_order;
    struct page *page;
    int migratetype, i;

    //Find the largest possible block of pages in the other list 
    for (current_order = MAX_ORDER-1; current_order >= order;
                        --current_order) {
        for (i = 0; i < MIGRATE_TYPES - 1; i++) {
            migratetype = fallbacks[start_migratetype][i];

            //MIGRATE_RESERVE handled later if necessary 
            if (migratetype == MIGRATE_RESERVE)
                continue;

            area = &(zone->free_area[current_order]);
            if (list_empty(&area->free_list[migratetype]))
                continue;

            page = list_entry(area->free_list[migratetype].next,
                    struct page, lru);
            area->nr_free--;


             //If breaking a large block of pages, move all free
             //pages to the preferred allocation list. If falling
             //back for a reclaimable kernel allocation, be more
             //agressive about taking ownership of free pages
            if (unlikely(current_order >= (pageblock_order >> 1)) ||
                    start_migratetype == MIGRATE_RECLAIMABLE) {
                unsigned long pages;
                pages = move_freepages_block(zone, page,
                                start_migratetype);

                //Claim the whole block if over half of it is free 
                if (pages >= (1 << (pageblock_order-1)))
                    set_pageblock_migratetype(page,
                                start_migratetype);

                migratetype = start_migratetype;
            }

           //Remove the page from the freelists 
            list_del(&page->lru);
            rmv_page_order(page);
            __mod_zone_page_state(zone, NR_FREE_PAGES,
                            -(1UL << order));                        


            if (current_order == pageblock_order)
                set_pageblock_migratetype(page,
                            start_migratetype);

            expand(zone, page, order, current_order, area, migratetype);
            return page;
        }
    }

    //Use MIGRATE_RESERVE rather than fail an allocation
    return __rmqueue_smallest(zone, order, MIGRATE_RESERVE);
}
*/





struct page *__alloc_pages_slowpath(unsigned int order, struct zonelist *zonelist)
{
	struct page *page = 0;

	unsigned long pages_reclaimed = 0;
	unsigned long did_some_progress;
	
	/* 
	 * In the slowpath, we sanity check order to avoid ever trying to 
	 * reclaim >= MAX_ORDER areas which will never succeed. Callers may 
	 * be using allocators in order of preference for an area that is 
	 * too large. 
	 *//*参数合法性检查*/
	if (order >= MAX_ORDER) {  
		return 0;  
	}  

	/* 
	 * GFP_THISNODE (meaning __GFP_THISNODE, __GFP_NORETRY and 
	 * __GFP_NOWARN set) should not cause reclaim since the subsystem 
	 * (f.e. slab) using GFP_THISNODE may choose to trigger reclaim 
	 * using a larger set of nodes after it has established that the 
	 * allowed per node queues are empty and that nodes are 
	 * over allocated. 
	 */  

  
	// Try direct reclaim and then allocating */  
	// 
	//直接在内存分配上下文中进行内存回收操作。 
	//	
	page = __alloc_pages_direct_reclaim(order);  
	if (page)// 庆幸，回收了一些内存后，满足了上层分配需求
		goto got_pg;  
  /*
	// 
	// If we failed to make any progress reclaiming, then we are 
	// running out of options and have to consider going OOM 
	//  
	//内存回收过程没有回收到内存，系统真的内存不足了
	if (!did_some_progress) {
		//
		// 调用者不是文件系统的代码，允许进行文件系统操作，并且允许重试。  
		// 这里需要__GFP_FS标志可能是进入OOM流程后会杀进程或进入panic，需要文件操作。 
		//  
		if ((gfp_mask & __GFP_FS) && !(gfp_mask & __GFP_NORETRY)) {  
			if (oom_killer_disabled)// 系统禁止了OOM，向上层返回NULL 
				goto nopage;  
			//
			// 杀死其他进程后再尝试分配内存 
			// 
			page = __alloc_pages_may_oom(gfp_mask, order,  
					zonelist, high_zoneidx,  
					nodemask, preferred_zone,  
					migratetype);  
			if (page)  
				goto got_pg;  
  
			//
			// The OOM killer does not trigger for high-order 
			// ~__GFP_NOFAIL allocations so if no progress is being 
			// made, there are no other options and retrying is 
			// unlikely to help. 
			// 要求的页面数量较多，再试意义不大 
			if (order > PAGE_ALLOC_COSTLY_ORDER &&	
						!(gfp_mask & __GFP_NOFAIL))  
				goto nopage;  
  
			goto restart;  
		}  
	}  
	
  
	// Check if we should retry the allocation 
 	// 内存回收过程回收了一些内存，接下来判断是否有必要继续重试
	pages_reclaimed += did_some_progress;  
	if (should_alloc_retry(gfp_mask, order, pages_reclaimed)) {  
		//Wait for some write requests to complete then retry 
		congestion_wait(BLK_RW_ASYNC, HZ/50);  
		goto rebalance;  
	}  
	*/
  
nopage:  
//内存分配失败了，打印内存分配失败的警告 
/*
	if (!(gfp_mask & __GFP_NOWARN) && printk_ratelimit()) {  
		printk(KERN_WARNING "%s: page allocation failure."	
			" order:%d, mode:0x%x\n",  
			p->comm, order, gfp_mask);	
		dump_stack();  
		show_mem();  
	}  
	*/
	return page;  
got_pg:  
	// 运行到这里，说明成功分配了内存，这里进行内存检测调试 
	
	return page;  
  

}

struct page *__alloc_pages_direct_reclaim(unsigned int order)
{
	//TODO
	return 0;
}


/* 
 * Go through the free lists for the given migratetype and remove 
 * the smallest available page from the freelists 
 */  
 /*从给定的order开始，从小到大遍历； 
  找到后返回页面基址，合并分割后的空间*/  
struct page *__rmqueue_smallest(struct zone *zone, unsigned int order)  
{  
	unsigned int current_order;  
	struct free_area * area;  
	struct page *page;	
  
	/* Find a page of the appropriate size in the preferred list */  
	for (current_order = order; current_order < MAX_ORDER; ++current_order) {  
		area = &(zone->free_area[current_order]);/*得到指定order的area*/  
		/*如果area指定类型的伙伴系统链表为空*/	
		if (list_empty(&area->free_list[0]))	
			continue;/*查找下一个order*/  
		/*对应的链表不空，得到链表中数据*/	
		page = list_entry(area->free_list[0].next,  
							struct page, lru);	
		list_del(&page->lru);/*从伙伴系统中删除；*/  
		set_page_private(page, 0);/*移除page中order的变量*/	
		area->nr_free--;/*空闲块数减一*/  
		/*拆分、合并*/	
		expand(zone, page, order, current_order, area);  
		
		return page;  
	}  
  
	return 0;  
}  




/* 
 * The order of subdivision here is critical for the IO subsystem. 
 * Please do not alter this order without good reasons and regression 
 * testing. Specifically, as large blocks of memory are subdivided, 
 * the order in which smaller blocks are delivered depends on the order 
 * they're subdivided in this function. This is the primary factor 
 * influencing the order in which pages are delivered to the IO 
 * subsystem according to empirical testing, and this is also justified 
 * by considering the behavior of a buddy system containing a single 
 * large block of memory acted on by a series of small allocations. 
 * This behavior is a critical factor in sglist merging's success. 
 * 
 * -- wli 
 */  
 /*此函数主要用于下面这种情况: 
  分配函数从high中分割出去了low大小的内存； 
  然后要将high留下的内存块合并放到伙伴系统中；*/  
void expand(struct zone *zone, struct page *page,  
	int low, int high, struct free_area *area)  
{  
	unsigned long size = 1 << high;  
  
	while (high > low) {/*因为去掉了low的大小，所以最后肯定剩下的 
	 是low的大小(2的指数运算)*/  
		area--;/*减一到order减一的area*/  
		high--;/*order减一*/  
		size >>= 1;/*大小除以2*/  
		//VM_BUG_ON(bad_range(zone, &page[size]));  
		/*加到指定的伙伴系统中*/  
		list_add(&page[size].lru, &area->free_list[0]);  
		area->nr_free++;/*空闲块加一*/	
		set_page_private(&page[size], high);/*设置相关order*/
	}  
}



void free_pages(unsigned long addr, unsigned int order)
{
	if (addr != 0) {  	
		__free_pages(virt_to_page(addr), order);/*具体的释放函数*/	
	}

}




void __free_pages(struct page *page, unsigned int order)
{
	if (put_page_testzero(page)) {/*count值减一为0时释放*/	
		/*调试*/  
		//trace_mm_page_free_direct(page, order);  
		if (order == 0)  
			free_hot_page(page);/*释放单个页面*/  
		else  
			__free_pages_ok(page, order);
	}  

}

/*
 * Methods to modify the page usage count.
 *
 * What counts for a page usage:
 * - cache mapping   (page->mapping)
 * - private data    (page->private)
 * - page mapped in a task's page tables, each mapping
 *   is counted separately
 *
 * Also, many kernel routines increase the page count before a critical
 * routine so they can be sure the page doesn't go away from under them.
 */

/*
 * Drop a ref, return true if the refcount fell to zero (the page has no users)
 */
int put_page_testzero(struct page *page)
{
    return atomic_dec_and_test(&page->_count);
} 

int atomic_dec_and_test(atomic_t *v)
{
    int ret,flags;
    //local_irq_save(flags);
    --(*v);
    ret = *v;
    //local_irq_restore(flags);
    return ret == 0;
} 

void free_hot_page(struct page *page)
{
    free_hot_cold_page(page, 0/* cold or hot */);                                                                            
}

/*                                                                                                          
 * Free a 0-order page
 */
void free_hot_cold_page(struct page *page, int cold)
{
    struct zone *zone = &(pg[0].node_zones[ZONE_NORMAL]);
    struct per_cpu_pages *pcp;
    unsigned long flags;

	/*
    if (PageAnon(page))
        page->mapping = NULL;
    if (free_pages_check(page))
        return;

    if (!PageHighMem(page)) {
        debug_check_no_locks_freed(page_address(page), PAGE_SIZE);
        debug_check_no_obj_freed(page_address(page), PAGE_SIZE);
    }
    arch_free_page(page, 0);
    kernel_map_pages(page, 1, 0);//trace
    */

	/*获得zone对应cpu的pcp*/  
    pcp = &zone_pcp(zone, 0).pcp;
    //local_irq_save(flags);
    //__count_vm_event(PGFREE);

    //cold page
    if (cold)
        list_add_tail(&page->lru, &pcp->list);
    else//hot page
        list_add(&page->lru, &pcp->list);
    set_page_private(page, 0);
    pcp->count++;
    if (pcp->count >= pcp->high) {
        free_pages_bulk(zone, pcp->batch, &pcp->list, 0);
        pcp->count -= pcp->batch;
    }          
    //local_irq_restore(flags);  
    //put_cpu(); 
}

/*
 * Frees a list of pages. 
 * Assumes all pages on list are in same zone, and of same order.
 * count is the number of pages to free.
 *
 * If the zone was previously in an "all pages pinned" state then look to
 * see if this freeing clears that state.
 *
 * And clear the zone's pages_scanned counter, to hold off the "all pages are
 * pinned" detection logic.
 */
void free_pages_bulk(struct zone *zone, int count,
                    struct list_head *list, int order)
{
    //spin_lock(&zone->lock);
    zone->pages_scanned = 0;
    while (count--) {
        struct page *page;

        //VM_BUG_ON(list_empty(list));
        page = list_entry(list->prev, struct page, lru);
        /* have to delete it as __free_one_page list manipulates */
        list_del(&page->lru);
        __free_one_page(page, zone, order);
    }   
    //spin_unlock(&zone->lock);
} 


/*
 * Freeing function for a buddy system allocator.
 *
 * The concept of a buddy system is to maintain direct-mapped table
 * (containing bit values) for memory blocks of various "orders".
 * The bottom level table contains the map for the smallest allocatable
 * units of memory (here, pages), and each level above it describes
 * pairs of units from the levels below, hence, "buddies".
 * At a high level, all that happens here is marking the table entry
 * at the bottom level available, and propagating the changes upward
 * as necessary, plus some accounting needed to play nicely with other
 * parts of the VM system.
 * At each level, we keep a list of pages, which are heads of continuous
 * free pages of length of (1 << order) and marked with PG_buddy. Page's
 * order is recorded in page_private(page) field.
 * So when we are allocating or freeing one, we can derive the state of the
 * other.  That is, if we allocate a small block, and both were   
 * free, the remainder of the region must be split into blocks.   
 * If a block is freed, and its buddy is also free, then this
 * triggers coalescing into a block of larger size.            
 *
 * -- wli
 */

void __free_one_page(struct page *page,
        struct zone *zone, unsigned int order)
{                                                                                                                                                                                                                                
    unsigned long page_idx;
    int order_size = 1 << order;
    int migratetype = 0;

	/*
    if (unlikely(PageCompound(page)))//要释放的页是巨页的一部分
    	//解决巨页标志，如果巨页标志有问题，则退出
        destroy_compound_page(page, order);
        */

	//将页面转化为全局页面数组的下标
    page_idx = page_to_pfn(page) & ((1 << MAX_ORDER) - 1);


    //__mod_zone_page_state(zone, NR_FREE_PAGES, order_size);
   
    while (order < MAX_ORDER-1) {
        unsigned long combined_idx;
        struct page *buddy;

		//找出page页面的伙伴
		//查找待释放页块的伙伴 ,其中伙伴系统中每个块
		//都有一个对应同样大小的"伙伴块"(由2的指数原理知)
        buddy = __page_find_buddy(page, page_idx, order);
        if (!page_is_buddy(page, buddy, order))
            break;

        /* Our buddy is free, merge with it and move up one order. */
        list_del(&buddy->lru);
        zone->free_area[order].nr_free--;
		set_page_private(buddy, 0);
		combined_idx = __find_combined_index(page_idx, order);
		page = page + (combined_idx - page_idx);
		page_idx = combined_idx;
		order++;
	}
	
	set_page_private(page, order);
	list_add(&page->lru,
		&zone->free_area[order].free_list[migratetype]);
	zone->free_area[order].nr_free++;
}

struct page *  
__page_find_buddy(struct page *page, unsigned long page_idx, unsigned int order)  
{  
  /*伙伴的计算原理， 
    *实际上，使用(1<<order)掩码的异或(XOR)转换page_idx第order位 
    *的值。因此，如果这个位原来是0，buddy_idx就等于page_idx+order 
    *相反，如果这个位原先是1，buddy_idx就等于page_idx-order 
    *此计算出来的伙伴为在mem_map中的下标 
    */    
    unsigned long buddy_idx = page_idx ^ (1 << order);  
    /*返回伙伴块的页面基址*/  
    return page + (buddy_idx - page_idx);  
}

int page_is_buddy(struct page *page, struct page *buddy, int order)  
{  
  
    //if (page_zone_id(page) != page_zone_id(buddy))/*验证page和他的buddy是否在同一个zone中*/  
    //    return 0;  
    /*验证相关位和buddy的order值*/  
    /* 通过检查PG_buddy 标志位判断buddy是否在伙伴系统中，并且buddy是否在order级的 
     链表中，page的private成员存放页块所在链表的order。*/  
    if (/* PageBuddy(buddy) && */page_order(buddy) == order) {  
        //VM_BUG_ON(page_count(buddy) != 0);  
        return 1;  
    }  
    return 0;  
}  

/*
 * function for dealing with page's order in buddy system.
 * zone->lock is already acquired when we use these.
 * So, we don't need atomic page->flags operations here.
 */
unsigned long page_order(struct page *page)
{
    return page_private(page);
} 


unsigned long
__find_combined_index(unsigned long page_idx, unsigned int order)                          
{
    return (page_idx & ~(1 << order));
}



void __free_pages_ok(struct page *page, unsigned int order)
{
	//TODO
} 



