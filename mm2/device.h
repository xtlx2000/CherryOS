#ifndef _DEVICE_H_
#define _DEVICE_H_

#include "macro.h"


typedef u32 __kernel_dev_t;

typedef __kernel_dev_t      dev_t;


struct block_device {
    dev_t           bd_dev;  /* not a kdev_t - it's a search key */
    struct inode *      bd_inode;   /* will die */
    int         bd_openers;
    //struct mutex        bd_mutex;   /* open/close mutex */
    //struct semaphore    bd_mount_sem;
    struct list_head    bd_inodes;
    void *          bd_holder;
    int         bd_holders;
#ifdef CONFIG_SYSFS
    struct list_head    bd_holder_list;
#endif
    struct block_device *   bd_contains;
    unsigned        bd_block_size;
    //struct hd_struct *  bd_part;
    /* number of times partitions within this device have been opened. */
    unsigned        bd_part_count;
    int         bd_invalidated;
    //struct gendisk *    bd_disk;
    struct list_head    bd_list;
    //struct backing_dev_info *bd_inode_backing_dev_info;
    /*
     * Private data.  You must have bd_claim'ed the block_device
     * to use this.  NOTE:  bd_claim allows an owner to claim
     * the same device multiple times, the owner must take special
     * care to not mess up bd_private for that case.
     */
    unsigned long       bd_private;
};


struct backing_dev_info {
    unsigned long ra_pages; /* max readahead in PAGE_CACHE_SIZE units */
    unsigned long state;    /* Always use atomic bitops on this */
    unsigned int capabilities; /* Device capabilities */
    //congested_fn *congested_fn; /* Function pointer if device is md/dm */
    void *congested_data;   /* Pointer to aux data for congested func */
    void (*unplug_io_fn)(struct backing_dev_info *, struct page *);
    void *unplug_io_data;

    //struct percpu_counter bdi_stat[NR_BDI_STAT_ITEMS];

    //struct prop_local_percpu completions;
    int dirty_exceeded;

    unsigned int min_ratio;
    unsigned int max_ratio, max_prop_frac;

    struct device *dev;

#ifdef CONFIG_DEBUG_FS
    struct dentry *debug_dir;
    struct dentry *debug_stats;
#endif
};



struct klist {                                                                                                                                                                                                                   
    spinlock_t      k_lock;
    struct list_head    k_list; 
    void            (*get)(struct klist_node *);
    void            (*put)(struct klist_node *);
};

struct klist_node {                                                                                                                                                                                                              
    struct klist        *n_klist;
    struct list_head    n_node; 
    //struct kref     n_ref;
    //struct completion   n_removed;
};

struct kobject {
    const char      *name;
    struct list_head    entry;
    struct kobject      *parent;
    struct kset     *kset;
    //struct kobj_type    *ktype;
    //struct sysfs_dirent *sd;
    //struct kref     kref;
    unsigned int state_initialized:1;
    unsigned int state_in_sysfs:1;
    unsigned int state_add_uevent_sent:1;
    unsigned int state_remove_uevent_sent:1;
};


struct device {
    struct klist        klist_children;
    struct klist_node   knode_parent;   /* node in sibling list */
    struct klist_node   knode_driver;
    struct klist_node   knode_bus;
    struct device       *parent;

    struct kobject kobj;
    //char    bus_id[BUS_ID_SIZE];    /* position on parent bus */
    const char      *init_name; /* initial name of the device */
    //struct device_type  *type;
    unsigned        uevent_suppress:1;

    //struct semaphore    sem;    /* semaphore to synchronize calls to
    //                 * its driver.
    //                 */

    //struct bus_type *bus;       /* type of bus device is on */
    //struct device_driver *driver;   /* which driver has allocated this
    //                   device */
    void        *driver_data;   /* data private to the driver */
    void        *platform_data; /* Platform specific data, device
                       core doesn't touch it */
    //struct dev_pm_info  power;

#ifdef CONFIG_NUMA
    int     numa_node;  /* NUMA node this device is close to */
#endif
    //u64     *dma_mask;  /* dma mask (if dma'able device) */
    //u64     coherent_dma_mask;/* Like dma_mask, but for
    //                     alloc_coherent mappings as
    //                     not all hardware supports
    //                     64 bit addresses for consistent
    //                     allocations such descriptors. */

    //struct device_dma_parameters *dma_parms;

    struct list_head    dma_pools;  /* dma pools (if dma'ble) */

    //struct dma_coherent_mem *dma_mem; /* internal for coherent mem override */
    /* arch specific additions */
    //struct dev_archdata archdata;

    spinlock_t      devres_lock;
    struct list_head    devres_head;

    struct list_head    node;
    struct class        *class;
    dev_t           devt;   /* dev_t, creates the sysfs "dev" */
    //struct attribute_group  **groups;   /* optional groups */         
    
    void    (*release)(struct device *dev);
};


#define MAX_DEVICE_NAMELEN	10

//OS中虚拟的存储设备
struct virtual_device
{
	char name[MAX_DEVICE_NAMELEN];
	void *address;
	u64 size;

	list_head dev_list;
};


#define VIRTUAL_DEVICE_SIZE  (4*1024*1024)//4MB


//我们虚拟出4个存储设备
struct virtual_device device_sda;
struct virtual_device device_sdb;
struct virtual_device device_sdc;
struct virtual_device device_sdd;

INIT_LIST_HEAD(device_list);
int device_init();
int init_virtual_device(struct virtual_device *dev, void *address, u64 size);

#endif
