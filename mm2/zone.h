

enum zone_type {
#ifdef CONFIG_ZONE_DMA
    /*
     * ZONE_DMA is used when there are devices that are not able
     * to do DMA to all of addressable memory (ZONE_NORMAL). Then we
     * carve out the portion of memory that is needed for these devices.
     * The range is arch specific.
     *
     * Some examples
     *
     * Architecture     Limit
     * ---------------------------
     * parisc, ia64, sparc  <4G
     * s390         <2G
     * arm          Various
     * alpha        Unlimited or 0-16MB.
     *
     * i386, x86_64 and multiple other arches
     *          <16M.
     */
    ZONE_DMA,
#endif
#ifdef CONFIG_ZONE_DMA32
    /*
     * x86_64 needs two ZONE_DMAs because it supports devices that are
     * only able to do DMA to the lower 16M but also 32 bit devices that
     * can only do DMA areas below 4G.
     */
    ZONE_DMA32,
#endif
    /*
     * Normal addressable memory is in ZONE_NORMAL. DMA operations can be
     * performed on pages in ZONE_NORMAL if the DMA devices support
     * transfers to all addressable memory.
     */
    ZONE_NORMAL,
#ifdef CONFIG_HIGHMEM
    /*
     * A memory area that is only addressable by the kernel through
     * mapping portions into its own address space. This is for example
     * used by i386 to allow the kernel to address the memory beyond
     * 900MB. The kernel will set up special mappings (page
     * table entries on i386) for each page that the kernel needs to
     * access.
     */
    ZONE_HIGHMEM,
#endif
    ZONE_MOVABLE,
    __MAX_NR_ZONES
};


struct free_area {                                                                                          
    struct list_head    free_list[MIGRATE_TYPES];
    unsigned long       nr_free;
};



struct zone {
    /* Fields commonly accessed by the page allocator */
    unsigned long       pages_min, pages_low, pages_high;
    /*   
     * We don't know if the memory that we're going to allocate will be freeable
     * or/and it will be released eventually, so to avoid totally wasting several
     * GB of ram we must reserve some of the lower zone memory (otherwise we risk
     * to run OOM on the lower zones despite there's tons of freeable ram
     * on the higher zones). This array is recalculated at runtime if the
     * sysctl_lowmem_reserve_ratio sysctl changes.
     */
    unsigned long       lowmem_reserve[MAX_NR_ZONES];

#ifdef CONFIG_NUMA
    int node;
    /*   
     * zone reclaim becomes active if more unmapped pages exist.
     */
    unsigned long       min_unmapped_pages;
    unsigned long       min_slab_pages;
    struct per_cpu_pageset  *pageset[NR_CPUS];
#else
    struct per_cpu_pageset  pageset[NR_CPUS];
#endif
    /*   
     * free areas of different sizes
     */
    spinlock_t      lock;
#ifdef CONFIG_MEMORY_HOTPLUG
    /* see spanned/present_pages for more description */
    seqlock_t       span_seqlock;
#endif
    struct free_area    free_area[MAX_ORDER];

#ifndef CONFIG_SPARSEMEM
    /*   
     * Flags for a pageblock_nr_pages block. See pageblock-flags.h.
     * In SPARSEMEM, this map is stored in struct mem_section
     */
    unsigned long       *pageblock_flags;
#endif /* CONFIG_SPARSEMEM */


    ZONE_PADDING(_pad1_)

    /* Fields commonly accessed by the page reclaim scanner */
    spinlock_t      lru_lock;   
    struct list_head    active_list;
    struct list_head    inactive_list;
    unsigned long       nr_scan_active;
    unsigned long       nr_scan_inactive;
    unsigned long       pages_scanned;     /* since last reclaim */                                     
    unsigned long       flags;         /* zone flags, see below */

    /* Zone statistics */
    atomic_long_t       vm_stat[NR_VM_ZONE_STAT_ITEMS];

    /*
     * prev_priority holds the scanning priority for this zone.  It is
     * defined as the scanning priority at which we achieved our reclaim
     * target at the previous try_to_free_pages() or balance_pgdat()
     * invokation.
     *
     * We use prev_priority as a measure of how much stress page reclaim is
     * under - it drives the swappiness decision: whether to unmap mapped
     * pages.
     *
     * Access to both this field is quite racy even on uniprocessor.  But
     * it is expected to average out OK.
     */
    int prev_priority;


    ZONE_PADDING(_pad2_)
    /* Rarely used or read-mostly fields */

    /*
     * wait_table       -- the array holding the hash table
     * wait_table_hash_nr_entries   -- the size of the hash table array
     * wait_table_bits  -- wait_table_size == (1 << wait_table_bits)
     *
     * The purpose of all these is to keep track of the people
     * waiting for a page to become available and make them
     * runnable again when possible. The trouble is that this
     * consumes a lot of space, especially when so few things
     * wait on pages at a given time. So instead of using
     * per-page waitqueues, we use a waitqueue hash table.
     *
     * The bucket discipline is to sleep on the same queue when
     * colliding and wake all in that wait queue when removing.
     * When something wakes, it must check to be sure its page is
     * truly available, a la thundering herd. The cost of a
     * collision is great, but given the expected load of the
     * table, they should be so rare as to be outweighed by the
     * benefits from the saved space.
     *
     * __wait_on_page_locked() and unlock_page() in mm/filemap.c, are the
     * primary users of these fields, and in mm/page_alloc.c
     * free_area_init_core() performs the initialization of them.
     */
    wait_queue_head_t   * wait_table;
    unsigned long       wait_table_hash_nr_entries;
    unsigned long       wait_table_bits;

    /*
     * Discontig memory support fields.
     */
    struct pglist_data  *zone_pgdat;
    /* zone_start_pfn == zone_start_paddr >> PAGE_SHIFT */
    unsigned long       zone_start_pfn;

    /*
     * zone_start_pfn, spanned_pages and present_pages are all
     * protected by span_seqlock.  It is a seqlock because it has
     * to be read outside of zone->lock, and it is done in the main
     * allocator path.  But, it is written quite infrequently.
     *
     * The lock is declared along with zone->lock because it is
     * frequently read in proximity to zone->lock.  It's good to
     * give them a chance of being in the same cacheline.
     */
    unsigned long       spanned_pages;  /* total size, including holes */
    unsigned long       present_pages;  /* amount of memory (excluding holes) */

    /*
     * rarely used fields:
     */
    const char      *name;
};


