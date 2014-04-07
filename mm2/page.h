#ifndef _PAGE_H_
#define _PAGE_H_

#include "macro.h"
#include "list.h"
/*
 * Each physical page in the system has a struct page associated with
 * it to keep track of whatever it is we are using the page for at the
 * moment. Note that we have no way to track which tasks are using
 * a page, though if it is a pagecache page, rmap structures can tell us
 * who is mapping it.
 */
struct page {
		unsigned long flags;		/* Atomic flags, some possibly
						 * updated asynchronously */
	atomic_t _count;		/* Usage count, see below. */
	union {
		atomic_t _mapcount; /* Count of ptes mapped in mms,
					 * to show when page is mapped
					 * & limit reverse map searches.
					 */
		struct {		/* SLUB */
			u16 inuse;
			u16 objects;
		};
	};
	union {
		struct {
		unsigned long private;		/* Mapping-private opaque data:
						 * usually used for buffer_heads
						 * if PagePrivate set; used for
						 * swp_entry_t if PageSwapCache;
						 * indicates order in the buddy
						 * system if PG_buddy is set.
						 */
		struct address_space *mapping;	/* If low bit clear, points to
						 * inode address_space, or NULL.
						 * If page mapped as anonymous
						 * memory, low bit is set, and
						 * it points to anon_vma object:
						 * see PAGE_MAPPING_ANON below.
						 */
		};
		
		struct kmem_cache *slab;	/* SLUB: Pointer to slab */
		struct page *first_page;	/* Compound tail pages */
	};
	union {
		pgoff_t index;		/* Our offset within mapping. */
		void *freelist; 	/* SLUB: freelist req. slab lock */
	};	
	struct list_head lru;		/* Pageout list, eg. active_list
					 * protected by zone->lru_lock !
					 */
	/*	
	* On machines where all RAM is mapped into kernel address space,
	* we can simply calculate the virtual address. On machines with
	* highmem some memory is mapped into kernel virtual memory
	* dynamically, so we need a place to store that address.
	* Note that this field could be 16 bits on x86 ... ;)
	*
	* Architectures with slow multiplication can define
	* WANT_PAGE_VIRTUAL in asm/page.h
	*/
	void *virtual;          /* Kernel virtual address (NULL if not kmapped, ie. highmem) */
	void *location;			/* physical memory address */

};



void *buffer;
struct page *mem_map;


#define PAGE_SHIFT	12

#define PAGESIZE	(1<<PAGE_SHIFT)


#define __pa(addr) 	\
	(((unsigned long)addr - (unsigned long)memory_pointer) / 4096)




#define page_private(page)      ((page)->private)
#define set_page_private(page, v)   ((page)->private = (v))


#define virt_to_page(kaddr)	\
	pfn_to_page(__pa(kaddr))



int paging_init(long detectsize);

struct page *pfn_to_page(unsigned long pfn);
unsigned long page_to_pfn(struct page *page);
struct page *pfn_to_page2(unsigned long pfn);
unsigned long page_to_pfn2(struct page *page);






#endif
