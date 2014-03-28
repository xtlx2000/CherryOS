#ifndef _ZONE_H_
#define _ZONE_H_

#include "macro.h"
#include "list.h"


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
    MAX_NR_ZONES
};


enum zone_stat_item {
    /* First 128 byte cacheline (assuming 64 bit words) */
    NR_FREE_PAGES,
    NR_INACTIVE,
    NR_ACTIVE,
    NR_ANON_PAGES,  /* Mapped anonymous pages */
    NR_FILE_MAPPED, /* pagecache pages mapped into pagetables.
               only modified from process context */
    NR_FILE_PAGES,
    NR_FILE_DIRTY,
    NR_WRITEBACK,
    /* Second 128 byte cacheline */
    NR_SLAB_RECLAIMABLE,
    NR_SLAB_UNRECLAIMABLE,
    NR_PAGETABLE,       /* used for pagetables */
    NR_UNSTABLE_NFS,    /* NFS unstable pages */
    NR_BOUNCE,
    NR_VMSCAN_WRITE,
    NR_WRITEBACK_TEMP,  /* Writeback using temporary buffers */
#ifdef CONFIG_NUMA
    NUMA_HIT,       /* allocated in intended node */
    NUMA_MISS,      /* allocated in non intended node */
    NUMA_FOREIGN,       /* was intended here, hit elsewhere */
    NUMA_INTERLEAVE_HIT,    /* interleaver preferred this zone */
    NUMA_LOCAL,     /* allocation from local node */
    NUMA_OTHER,     /* allocation from other node */
#endif
    NR_VM_ZONE_STAT_ITEMS };


#define MAX_ZONELISTS 2


#define MIGRATE_UNMOVABLE	 0
#define MIGRATE_RECLAIMABLE	 1
#define MIGRATE_MOVABLE		 2
#define MIGRATE_RESERVE		 3
#define MIGRATE_ISOLATE		 4 /* can't allocate from here */
#define MIGRATE_TYPES		 5	 


#define MAX_ORDER 11


/*
 * We cache key information from each zonelist for smaller cache
 * footprint when scanning for free pages in get_page_from_freelist().
 *
 * 1) The BITMAP fullzones tracks which zones in a zonelist have come
 *    up short of free memory since the last time (last_fullzone_zap)
 *    we zero'd fullzones.
 * 2) The array z_to_n[] maps each zone in the zonelist to its node
 *    id, so that we can efficiently evaluate whether that node is
 *    set in the current tasks mems_allowed.
 *
 * Both fullzones and z_to_n[] are one-to-one with the zonelist,
 * indexed by a zones offset in the zonelist zones[] array.
 *
 * The get_page_from_freelist() routine does two scans.  During the
 * first scan, we skip zones whose corresponding bit in 'fullzones'
 * is set or whose corresponding node in current->mems_allowed (which
 * comes from cpusets) is not set.  During the second scan, we bypass
 * this zonelist_cache, to ensure we look methodically at each zone.
 *
 * Once per second, we zero out (zap) fullzones, forcing us to
 * reconsider nodes that might have regained more free memory.
 * The field last_full_zap is the time we last zapped fullzones.
 *
 * This mechanism reduces the amount of time we waste repeatedly
 * reexaming zones for free memory when they just came up low on
 * memory momentarilly ago.
 *
 * The zonelist_cache struct members logically belong in struct
 * zonelist.  However, the mempolicy zonelists constructed for
 * MPOL_BIND are intentionally variable length (and usually much
 * shorter).  A general purpose mechanism for handling structs with
 * multiple variable length members is more mechanism than we want
 * here.  We resort to some special case hackery instead.
 *
 * The MPOL_BIND zonelists don't need this zonelist_cache (in good
 * part because they are shorter), so we put the fixed length stuff
 * at the front of the zonelist struct, ending in a variable length
 * zones[], as is needed by MPOL_BIND.
 *
 * Then we put the optional zonelist cache on the end of the zonelist
 * struct.  This optional stuff is found by a 'zlcache_ptr' pointer in
 * the fixed length portion at the front of the struct.  This pointer
 * both enables us to find the zonelist cache, and in the case of
 * MPOL_BIND zonelists, (which will just set the zlcache_ptr to NULL)
 * to know that the zonelist cache is not there.
 *
 * The end result is that struct zonelists come in two flavors:
 *  1) The full, fixed length version, shown below, and
 *  2) The custom zonelists for MPOL_BIND.
 * The custom MPOL_BIND zonelists have a NULL zlcache_ptr and no zlcache.
 *
 * Even though there may be multiple CPU cores on a node modifying
 * fullzones or last_full_zap in the same zonelist_cache at the same
 * time, we don't lock it.  This is just hint data - if it is wrong now
 * and then, the allocator will still function, perhaps a bit slower.
 */
struct zonelist_cache {
    //unsigned short z_to_n[MAX_ZONES_PER_ZONELIST];      /* zone->nid */
    //DECLARE_BITMAP(fullzones, MAX_ZONES_PER_ZONELIST);  /* zone full? */
    unsigned long last_full_zap;        /* when last zap'd (jiffies) */
};



/*
 * This struct contains information about a zone in a zonelist. It is stored
 * here to avoid dereferences into large structures and lookups of tables
 */
struct zoneref {
    struct zone *zone;  /* Pointer to actual zone */
    int zone_idx;       /* zone_idx(zoneref->zone) */
};




/*
 * One allocation request operates on a zonelist. A zonelist
 * is a list of zones, the first one is the 'goal' of the
 * allocation, the other zones are fallback zones, in decreasing
 * priority.
 *
 * If zlcache_ptr is not NULL, then it is just the address of zlcache,
 * as explained above.  If zlcache_ptr is NULL, there is no zlcache.
 * *
 * To speed the reading of the zonelist, the zonerefs contain the zone index
 * of the entry being read. Helper functions to access information given
 * a struct zoneref are
 *
 * zonelist_zone()  - Return the struct zone * for an entry in _zonerefs
 * zonelist_zone_idx()  - Return the index of the zone for an entry
 * zonelist_node_idx()  - Return the index of the node for an entry
 */
struct zonelist {                                                                                           
    struct zonelist_cache *zlcache_ptr;          // NULL or &zlcache
    //struct zoneref _zonerefs[MAX_ZONES_PER_ZONELIST + 1];
#ifdef CONFIG_NUMA
    struct zonelist_cache zlcache;               // optional ...
#endif
};





struct free_area {                                                                                          
    struct list_head    free_list[MIGRATE_TYPES];
    unsigned long       nr_free;
};

struct per_cpu_pages {																				   
	int count;		/* number of pages in the list */
	int high;		/* high watermark, emptying needed */
	int batch;		/* chunk size for buddy add/remove */
	struct list_head list;	/* the list of pages */
};


struct per_cpu_pageset {																			   
	struct per_cpu_pages pcp;
#ifdef CONFIG_NUMA
	s8 expire;
#endif
#ifdef CONFIG_SMP
	s8 stat_threshold;
	s8 vm_stat_diff[NR_VM_ZONE_STAT_ITEMS];
#endif
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


    /* Fields commonly accessed by the page reclaim scanner */
    spinlock_t      lru_lock;   
    struct list_head    active_list;
    struct list_head    inactive_list;
    unsigned long       nr_scan_active;
    unsigned long       nr_scan_inactive;
    unsigned long       pages_scanned;     /* since last reclaim */                                     
    unsigned long       flags;         /* zone flags, see below */

    /* Zone statistics */
    atomic_t       vm_stat[NR_VM_ZONE_STAT_ITEMS];

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
    //wait_queue_head_t   * wait_table;
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


int zone_init(struct zone *z);

#endif
