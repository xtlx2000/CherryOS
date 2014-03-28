#ifndef _PG_DATA_H_
#define _PG_DATA_H_

#include "zone.h"

/*
 * The pg_data_t structure is used in machines with CONFIG_DISCONTIGMEM
 * (mostly NUMA machines?) to denote a higher-level memory zone than the
 * zone denotes.
 *
 * On NUMA machines, each NUMA node would have a pg_data_t to describe
 * it's memory layout.
 *
 * Memory statistics and page replacement data structures are maintained on a
 * per-zone basis.      
 */
typedef struct pglist_data {                                                                                                                                                                                                     
    struct zone node_zones[MAX_NR_ZONES];
    struct zonelist node_zonelists[MAX_ZONELISTS];
    int nr_zones;
    struct page *node_mem_map;
    
#ifdef CONFIG_MEMORY_HOTPLUG
    /*
     * Must be held any time you expect node_start_pfn, node_present_pages
     * or node_spanned_pages stay constant.  Holding this will also
     * guarantee that any pfn_valid() stays that way.
     *
     * Nests above zone->lock and zone->size_seqlock.
     */
    spinlock_t node_size_lock;
    
#endif
    unsigned long node_start_pfn; 
    unsigned long node_present_pages; /* total number of physical pages */
    unsigned long node_spanned_pages; /* total size of physical page range, including holes */
    int node_id;
    //wait_queue_head_t kswapd_wait;
    struct task_struct *kswapd;
    int kswapd_max_order;
} pg_data_t;



int pg_data_init();



#endif
