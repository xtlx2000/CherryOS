#ifndef _VM_H_
#define _VM_H_

#include "prio_tree.h"
#include "rbtree.h"


struct vm_area_struct;



/*
 * vm_fault is filled by the the pagefault handler and passed to the vma's
 * ->fault function. The vma's ->fault is responsible for returning a bitmask
 * of VM_FAULT_xxx flags that give details about how the fault was handled.
 *
 * pgoff should be used in favour of virtual_address, if possible. If pgoff
 * is used, one may set VM_CAN_NONLINEAR in the vma->vm_flags to get nonlinear
 * mapping support.
 */
struct vm_fault {
    unsigned int flags;     /* FAULT_FLAG_xxx flags */
    pgoff_t pgoff;          /* Logical page offset based on vma */
    void *virtual_address;   /* Faulting virtual address */

    struct page *page;      /* ->fault handlers should return a
                     * page here, unless VM_FAULT_NOPAGE
                     * is set (which is also implied by
                     * VM_FAULT_ERROR).
                     */ 
}; 



/*
 * These are the virtual MM functions - opening of an area, closing and
 * unmapping it (needed to keep files on disk up-to-date etc), pointer
 * to the functions called when a no-page or a wp-page exception occurs. 
 */
struct vm_operations_struct {
    void (*open)(struct vm_area_struct * area);
    void (*close)(struct vm_area_struct * area);
    int (*fault)(struct vm_area_struct *vma, struct vm_fault *vmf);

    /* notification that a previously read-only page is about to become
     * writable, if an error is returned it will cause a SIGBUS */
    int (*page_mkwrite)(struct vm_area_struct *vma, struct page *page);

    /* called by access_process_vm when get_user_pages() fails, typically
     * for use by special VMAs that can switch between memory and hardware
     */
    int (*access)(struct vm_area_struct *vma, unsigned long addr,
              void *buf, int len, int write);
#ifdef CONFIG_NUMA
    /*
     * set_policy() op must add a reference to any non-NULL @new mempolicy
     * to hold the policy upon return.  Caller should pass NULL @new to
     * remove a policy and fall back to surrounding context--i.e. do not
     * install a MPOL_DEFAULT policy, nor the task or system default
     * mempolicy.
     */
    //int (*set_policy)(struct vm_area_struct *vma, struct mempolicy *new);

    /*
     * get_policy() op must add reference [mpol_get()] to any policy at
     * (vma,addr) marked as MPOL_SHARED.  The shared policy infrastructure
     * in mm/mempolicy.c will do this automatically.
     * get_policy() must NOT add a ref if the policy at (vma,addr) is not
     * marked as MPOL_SHARED. vma policies are protected by the mmap_sem.
     * If no [shared/vma] mempolicy exists at the addr, get_policy() op
     * must return NULL--i.e., do not "fallback" to task or system default
     * policy.
     */
    //struct mempolicy *(*get_policy)(struct vm_area_struct *vma,
    //                unsigned long addr);
    //int (*migrate)(struct vm_area_struct *vma, const nodemask_t *from,
    //    const nodemask_t *to, unsigned long flags);
#endif
};




/*
 * This struct defines a memory VMM memory area. There is one of these
 * per VM-area/task.  A VM area is any part of the process virtual memory
 * space that has a special rule for the page-fault handlers (ie a shared
 * library, the executable area etc).
 */
struct vm_area_struct {
    //struct mm_struct * vm_mm;   /* The address space we belong to. */
    unsigned long vm_start;     /* Our start address within vm_mm. */
    unsigned long vm_end;       /* The first byte after our end address
                       within vm_mm. */

    /* linked list of VM areas per task, sorted by address */
    struct vm_area_struct *vm_next;

    u64 vm_page_prot;      /* Access permissions of this VMA. */
    unsigned long vm_flags;     /* Flags, see mm.h. */

    struct rb_node vm_rb;

    /*  
     * For areas with an address space and backing store,
     * linkage into the address_space->i_mmap prio tree, or
     * linkage to the list of like vmas hanging off its node, or
     * linkage of vma in the address_space->i_mmap_nonlinear list.
     */
    union {
        struct {
            struct list_head list;
            void *parent;   /* aligns with prio_tree_node parent */
            struct vm_area_struct *head;
        } vm_set;

        struct raw_prio_tree_node prio_tree_node;
    } shared;

    /*  
     * A file's MAP_PRIVATE vma can be in both i_mmap tree and anon_vma
     * list, after a COW of one of the file pages.  A MAP_SHARED vma
     * can only be in the i_mmap tree.  An anonymous MAP_PRIVATE, stack
     * or brk vma (with NULL file) can only be in an anon_vma list.
     */
    struct list_head anon_vma_node; /* Serialized by anon_vma->lock */
    struct anon_vma *anon_vma;  /* Serialized by page_table_lock */

    /* Function pointers to deal with this struct. */
    struct vm_operations_struct * vm_ops;

    /* Information about our backing store: */
    unsigned long vm_pgoff;     /* Offset (within vm_file) in PAGE_SIZE
                       units, *not* PAGE_CACHE_SIZE */
    struct file * vm_file;      /* File we map to (can be NULL). */
    void * vm_private_data;     /* was vm_pte (shared mem) */
    unsigned long vm_truncate_count;/* truncate_count or restart_addr */
	
#ifndef CONFIG_MMU
	atomic_t vm_usage;		/* refcount (VMAs shared if !MMU) */
#endif
#ifdef CONFIG_NUMA
	struct mempolicy *vm_policy;	/* NUMA policy for the VMA */
#endif
};




/*  
 * A control structure which tells the writeback code what to do.  These are
 * always on the stack, and hence need no locking.  They are always initialised
 * in a manner such that unspecified fields are set to zero.
 */                     
struct writeback_control {
    struct backing_dev_info *bdi;   /* If !NULL, only write back this
                       queue */
    //enum writeback_sync_modes sync_mode;
    unsigned long *older_than_this; /* If !NULL, only write back inodes
                       older than this */
    long nr_to_write;       /* Write this many pages, and decrement
                       this for each page written */
    long pages_skipped;     /* Pages which were not written */

    /*
     * For a_ops->writepages(): is start or end are non-zero then this is
     * a hint that the filesystem need only write out the pages inside that
     * byterange.  The byte at `end' is included in the writeout request.
     */
    loff_t range_start;
    loff_t range_end;

    unsigned nonblocking:1;     /* Don't get stuck on request queues */
    unsigned encountered_congestion:1; /* An output: a queue is full */
    unsigned for_kupdate:1;     /* A kupdate writeback */
    unsigned for_reclaim:1;     /* Invoked from the page allocator */
    unsigned for_writepages:1;  /* This is a writepages() call */
    unsigned range_cyclic:1;    /* range_start is cyclic */
    unsigned more_io:1;     /* more io to be dispatched */
    unsigned range_cont:1;
};




struct address_space_operations {
    int (*writepage)(struct page *page, struct writeback_control *wbc);
    int (*readpage)(struct file *, struct page *);
    void (*sync_page)(struct page *);

    /* Write back some dirty pages from this mapping. */
    int (*writepages)(struct address_space *, struct writeback_control *);

    /* Set a page dirty.  Return true if this dirtied it */
    int (*set_page_dirty)(struct page *page);

    int (*readpages)(struct file *filp, struct address_space *mapping,
            struct list_head *pages, unsigned nr_pages);

    /*
     * ext3 requires that a successful prepare_write() call be followed
     * by a commit_write() call - they must be balanced
     */
    int (*prepare_write)(struct file *, struct page *, unsigned, unsigned);
    int (*commit_write)(struct file *, struct page *, unsigned, unsigned);

    int (*write_begin)(struct file *, struct address_space *mapping,
                loff_t pos, unsigned len, unsigned flags,
                struct page **pagep, void **fsdata);
    int (*write_end)(struct file *, struct address_space *mapping,
                loff_t pos, unsigned len, unsigned copied,
                struct page *page, void *fsdata);

    /* Unfortunately this kludge is needed for FIBMAP. Don't use it */
    //sector_t (*bmap)(struct address_space *, sector_t);
    void (*invalidatepage) (struct page *, unsigned long);
    int (*releasepage) (struct page *, gfp_t);
    ssize_t (*direct_IO)(int, struct kiocb *, const struct iovec *iov,
            loff_t offset, unsigned long nr_segs);
    int (*get_xip_mem)(struct address_space *, pgoff_t, int,
                        void **, unsigned long *);
    /* migrate the contents of a page to the specified target */
    int (*migratepage) (struct address_space *,
            struct page *, struct page *);
    int (*launder_page) (struct page *);
    //int (*is_partially_uptodate) (struct page *, read_descriptor_t *,
    //                unsigned long);
};


#endif
