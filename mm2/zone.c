#include "zone.h"
#include "list.h"

int per_cpu_pages_init(struct per_cpu_pageset *pageset);
int zone_init_free_lists(struct zone *z);
struct page *alloc_pages_current(gfp_t gfp, unsigned order);
struct page *__alloc_pages_node(gfp_t gfp_mask, unsigned int order, struct zonelist *zonelist);
static struct page *get_page_from_freelist(unsigned int order,struct zonelist *zonelist, int high_zoneidx, int alloc_flags);
struct page *buffered_rmqueue(struct zone *preferred_zone, struct zone *zone, int order, gfp_t gfp_flags);
struct page *__alloc_pages_slowpath(unsigned int order,struct zonelist *zonelist, enum zone_type high_zoneidx, struct zone *preferred_zone,int migratetype);
struct page *__rmqueue_smallest(struct zone *zone, unsigned int order, int migratetype);
void expand(struct zone *zone, struct page *page, int low, int high, struct free_area *area);






int zone_init(struct zone *z)
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

	//init the buddy system
	zone_init_free_lists(z);

	//add ram to buddy
	

	
	
	return 0;
}

#define for_each_migratetype_order(order, type) \
	for (order = 0; order < MAX_ORDER; order++) \
		for (type = 0; type < MIGRATE_TYPES; type++)


int zone_init_free_lists(struct zone *z)
{
	int order, t;
	for_each_migratetype_order(order, t) {
		INIT_LIST_HEAD(&zone->free_area[order].free_list[t]);  //��ʼ������
		zone->free_area[order].nr_free = 0;  //��ʼ��nr_freeΪ0
	}
}

int per_cpu_pages_init(struct per_cpu_pageset *pageset)
{
	pageset->pcp.count = 0;		/* number of pages in the list */
	pageset->pcp.high;		/* high watermark, emptying needed */
	pageset->pcp.batch;		/* chunk size for buddy add/remove */
	INIT_LIST_HEAD(&pageset->pcp.list);	/* the list of pages */
	INIT_LIST_HEAD(&pageset->list);
}








static inline struct page *
alloc_pages(gfp_t gfp_mask, unsigned int order)                                                         
{    
    if(order >= MAX_ORDER)
        return NULL;

    return alloc_pages_current(gfp_mask, order);
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
struct page *alloc_pages_current(gfp_t gfp, unsigned order)                                                 
{
	//TODO: current
    pg_data_t* pg_data = &pg[0];

    return __alloc_pages_nodemask(gfp_t gfp_mask, unsigned int order,	
			struct zonelist *zonelist, nodemask_t *nodemask);
    
}


/*
 * This is the 'heart' of the zoned buddy allocator.
 */
struct page *  
__alloc_pages_node(gfp_t gfp_mask, unsigned int order, struct zonelist *zonelist)
{
	struct page *page;
	/* health check */
	//TODO
	
	/* First allocation attempt : alloc from pcp */
	page = get_page_from_freelist(order, zonelist, high_zoneidx, ALLOC_WMARK_LOW|ALLOC_CPUSET,  
					preferred_zone, migratetype);

	/* slow allocation */
	if(!page){
		
		page = __alloc_pages_slowpath(gfp_mask, order,  
							zonelist, high_zoneidx, nodemask,  
							preferred_zone, migratetype);
	}
	
}


static struct page *
get_page_from_freelist(unsigned int order,struct zonelist *zonelist, int high_zoneidx, int alloc_flags)
{
	struct page *page;
	page = buffered_rmqueue(preferred_zone, zone, order);

	return page;
}


/*
 * Really, prep_compound_page() should be called from __rmqueue_bulk().  But
 * we cheat by calling it from here, in the order > 0 path.  Saves a branch
 * or two.
 */
struct page *buffered_rmqueue(struct zone *preferred_zone,
            struct zone *zone, int order, gfp_t gfp_flags)
{
    unsigned long flags;
    struct page *page;
    int cold = !!(gfp_flags & __GFP_COLD);
    int cpu;
    int migratetype = allocflags_to_migratetype(gfp_flags);

again:
    cpu  = get_cpu();
    if (likely(order == 0)) {
        struct per_cpu_pages *pcp;

        pcp = &zone_pcp(zone, cpu)->pcp;
        local_irq_save(flags);
        if (!pcp->count) {
            pcp->count = rmqueue_bulk(zone, 0,
                    pcp->batch, &pcp->list, migratetype);
            if (unlikely(!pcp->count))
                goto failed;
        }

        /* Find a page of the appropriate migrate type */
        if (cold) {
            list_for_each_entry_reverse(page, &pcp->list, lru)
                if (page_private(page) == migratetype)
                    break;
        } else {
            list_for_each_entry(page, &pcp->list, lru)
                if (page_private(page) == migratetype)
                    break;
        }

        /* Allocate more to the pcp list if necessary */
        if (unlikely(&page->lru == &pcp->list)) {
            pcp->count += rmqueue_bulk(zone, 0,
                    pcp->batch, &pcp->list, migratetype);
            page = list_entry(pcp->list.next, struct page, lru);
        }

        list_del(&page->lru);
        pcp->count--;
    } else {
        spin_lock_irqsave(&zone->lock, flags);
        page = __rmqueue(zone, order, migratetype);
        spin_unlock(&zone->lock);                            
        if (!page)
            goto failed;
    }

    __count_zone_vm_events(PGALLOC, zone, 1 << order);
    zone_statistics(preferred_zone, zone);
    local_irq_restore(flags);
    put_cpu();

    VM_BUG_ON(bad_range(zone, page));
    if (prep_new_page(page, order, gfp_flags))
        goto again;
    return page;

failed:
    local_irq_restore(flags);
    put_cpu();
    return NULL;
}


struct page *__alloc_pages_slowpath(unsigned int order,struct zonelist *zonelist, enum zone_type high_zoneidx, struct zone *preferred_zone,int migratetype)
{
	struct page *page = NULL;

	unsigned long pages_reclaimed = 0;
	unsigned long did_some_progress;
	
	/* 
	 * In the slowpath, we sanity check order to avoid ever trying to 
	 * reclaim >= MAX_ORDER areas which will never succeed. Callers may 
	 * be using allocators in order of preference for an area that is 
	 * too large. 
	 *//*�����Ϸ��Լ��*/  
	if (order >= MAX_ORDER) {  
		WARN_ON_ONCE(!(gfp_mask & __GFP_NOWARN));  
		return NULL;  
	}  

	/* 
	 * GFP_THISNODE (meaning __GFP_THISNODE, __GFP_NORETRY and 
	 * __GFP_NOWARN set) should not cause reclaim since the subsystem 
	 * (f.e. slab) using GFP_THISNODE may choose to trigger reclaim 
	 * using a larger set of nodes after it has established that the 
	 * allowed per node queues are empty and that nodes are 
	 * over allocated. 
	 */  
		 /** 
		  * ������ָ����GFP_THISNODE��־����ʾ���ܽ����ڴ���ա� 
		  * �ϲ������Ӧ����ָ����GFP_THISNODEʧ�ܺ�ʹ��������־���з��䡣 
		  */  
	if (NUMA_BUILD && (gfp_mask & GFP_THISNODE) == GFP_THISNODE)  
		goto nopage;  
  
restart:/*���������û�н�ֹkswapd�����Ѹ��߳̽����ڴ���ա�*/  
	wake_all_kswapd(order, zonelist, high_zoneidx);  
  
	/* 
	 * OK, we're below the kswapd watermark and have kicked background 
	 * reclaim. Now things get more complex, so set up alloc_flags according 
	 * to how we want to proceed. 
	 */  
	 /*���ݷ����־ȷ���ڲ���־����Ҫ������ˮ�� */	
	alloc_flags = gfp_to_alloc_flags(gfp_mask);  
  
		/** 
		  * ����ٷ���������ȣ�����ķ����־ʹ���˵͵�ˮ�ߡ� 
		  * �ڽ����ڴ���ղ���ǰ������ʹ�õ�ˮ���ٳ��Է���һ�¡� 
		  * ��Ȼ�������Ƿ�����ALLOC_NO_WATERMARKS��־�����Ƕ���������� 
		  */  
	/* This is the last chance, in general, before the goto nopage. */	
	page = get_page_from_freelist(gfp_mask, nodemask, order, zonelist,	
			high_zoneidx, alloc_flags & ~ALLOC_NO_WATERMARKS,  
			preferred_zone, migratetype);  
	if (page)/*����ɹ����ҵ�ҳ��*/  
		goto got_pg;  
  
rebalance:	
	/* Allocate without watermarks if the context allows */  
/* ĳЩ�����ģ����ڴ���ս��̼���ɱ�������񣬶���������ȫͻ��ˮ�ߵ����Ʒ����ڴ档 */  
	if (alloc_flags & ALLOC_NO_WATERMARKS) {  
		page = __alloc_pages_high_priority(gfp_mask, order,  
				zonelist, high_zoneidx, nodemask,  
				preferred_zone, migratetype);  
		if (page))/* �ڲ�����ˮ�ߵ�����£����䵽���ڴ� */	
			goto got_pg;  
	}  
  
	/* Atomic allocations - we can't balance anything */  
	/* ������ϣ��ԭ�ӷ����ڴ棬��ʱ���ܵȴ��ڴ���գ�����NULL */  
	if (!wait)	
		goto nopage;  
  
	/* Avoid recursion of direct reclaim */  
/* �����߱�������ڴ���ս��̣����ܽ��������ڴ���մ������̣��������� */	
	if (p->flags & PF_MEMALLOC)  
		goto nopage;  
  
	/* Avoid allocations with no watermarks from looping endlessly */  
	/** 
	* ��ǰ�߳����ڱ�ɱ������������ȫͻ��ˮ�߷����ڴ档�������ϲ㷵��NULL����Ϊ�˱���ϵͳ������ѭ���� 
	* ��Ȼ������ϲ���ò�����ʧ�ܣ�����ѭ���������䣬�ȴ������߳��ͷ�һ����ڴ档 
	*/	
	if (test_thread_flag(TIF_MEMDIE) && !(gfp_mask & __GFP_NOFAIL))  
		goto nopage;  
  
	/* Try direct reclaim and then allocating */  
	/** 
	* ֱ�����ڴ�����������н����ڴ���ղ����� 
	*/	
	page = __alloc_pages_direct_reclaim(gfp_mask, order,  
					zonelist, high_zoneidx,  
					nodemask,  
					alloc_flags, preferred_zone,  
					migratetype, &did_some_progress);  
	if (page))/* ���ң�������һЩ�ڴ���������ϲ�������� */	
		goto got_pg;  
  
	/* 
	 * If we failed to make any progress reclaiming, then we are 
	 * running out of options and have to consider going OOM 
	 */  
	/* �ڴ���չ���û�л��յ��ڴ棬ϵͳ����ڴ治���� */  
	if (!did_some_progress) {  
		/** 
		 * �����߲����ļ�ϵͳ�Ĵ��룬��������ļ�ϵͳ�����������������ԡ�  
		 * ������Ҫ__GFP_FS��־�����ǽ���OOM���̺��ɱ���̻����panic����Ҫ�ļ������� 
		 */  
		if ((gfp_mask & __GFP_FS) && !(gfp_mask & __GFP_NORETRY)) {  
			if (oom_killer_disabled)/* ϵͳ��ֹ��OOM�����ϲ㷵��NULL */  
				goto nopage;  
			/** 
			 * ɱ���������̺��ٳ��Է����ڴ� 
			 */  
			page = __alloc_pages_may_oom(gfp_mask, order,  
					zonelist, high_zoneidx,  
					nodemask, preferred_zone,  
					migratetype);  
			if (page)  
				goto got_pg;  
  
			/* 
			 * The OOM killer does not trigger for high-order 
			 * ~__GFP_NOFAIL allocations so if no progress is being 
			 * made, there are no other options and retrying is 
			 * unlikely to help. 
			 */)/* Ҫ���ҳ�������϶࣬�������岻�� */	
			if (order > PAGE_ALLOC_COSTLY_ORDER &&	
						!(gfp_mask & __GFP_NOFAIL))  
				goto nopage;  
  
			goto restart;  
		}  
	}  
  
	/* Check if we should retry the allocation */  
 /* �ڴ���չ��̻�����һЩ�ڴ棬�������ж��Ƿ��б�Ҫ�������� */  
	pages_reclaimed += did_some_progress;  
	if (should_alloc_retry(gfp_mask, order, pages_reclaimed)) {  
		/* Wait for some write requests to complete then retry */  
		congestion_wait(BLK_RW_ASYNC, HZ/50);  
		goto rebalance;  
	}  
  
nopage:  
/* �ڴ����ʧ���ˣ���ӡ�ڴ����ʧ�ܵľ��� */  
	if (!(gfp_mask & __GFP_NOWARN) && printk_ratelimit()) {  
		printk(KERN_WARNING "%s: page allocation failure."	
			" order:%d, mode:0x%x\n",  
			p->comm, order, gfp_mask);	
		dump_stack();  
		show_mem();  
	}  
	return page;  
got_pg:  
	/* ���е����˵���ɹ��������ڴ棬��������ڴ������ */	
	if (kmemcheck_enabled)	
		kmemcheck_pagealloc_alloc(page, order, gfp_mask);  
	return page;  
  

}


/* 
 * Go through the free lists for the given migratetype and remove 
 * the smallest available page from the freelists 
 */  
 /*�Ӹ�����order��ʼ����С��������� 
  �ҵ��󷵻�ҳ���ַ���ϲ��ָ��Ŀռ�*/  
static inline  
struct page *__rmqueue_smallest(struct zone *zone, unsigned int order,	
						int migratetype)  
{  
	unsigned int current_order;  
	struct free_area * area;  
	struct page *page;	
  
	/* Find a page of the appropriate size in the preferred list */  
	for (current_order = order; current_order < MAX_ORDER; ++current_order) {  
		area = &(zone->free_area[current_order]);/*�õ�ָ��order��area*/  
		/*���areaָ�����͵Ļ��ϵͳ����Ϊ��*/	
		if (list_empty(&area->free_list[migratetype]))	
			continue;/*������һ��order*/  
		/*��Ӧ�������գ��õ�����������*/	
		page = list_entry(area->free_list[migratetype].next,  
							struct page, lru);	
		list_del(&page->lru);/*�ӻ��ϵͳ��ɾ����*/  
		rmv_page_order(page);/*�Ƴ�page��order�ı���*/	
		area->nr_free--;/*���п�����һ*/  
		/*��֡��ϲ�*/	
		expand(zone, page, order, current_order, area, migratetype);  
		
		return page;  
	}  
  
	return NULL;  
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
 /*�˺�����Ҫ���������������: 
  ���亯����high�зָ��ȥ��low��С���ڴ棻 
  Ȼ��Ҫ��high���µ��ڴ��ϲ��ŵ����ϵͳ�У�*/  
static inline void expand(struct zone *zone, struct page *page,  
	int low, int high, struct free_area *area)  
{  
	unsigned long size = 1 << high;  
  
	while (high > low) {/*��Ϊȥ����low�Ĵ�С���������϶�ʣ�µ� 
	 ��low�Ĵ�С(2��ָ������)*/  
		area--;/*��һ��order��һ��area*/  
		high--;/*order��һ*/  
		size >>= 1;/*��С����2*/  
		VM_BUG_ON(bad_range(zone, &page[size]));  
		/*�ӵ�ָ���Ļ��ϵͳ��*/  
		list_add(&page[size].lru, &area->free_list[migratetype]);  
		area->nr_free++;/*���п��һ*/	
		set_page_order(&page[size], high);/*�������order*/  
	}  
}  

