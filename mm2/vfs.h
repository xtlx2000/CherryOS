#ifndef _VFS_H_
#define _VFS_H_

#include "device.h"

#include "radix.h"
#include "prio_tree.h"


struct path {                                                                                               
    struct vfsmount *mnt;
    struct dentry *dentry;
};


struct fs_struct {                                                                                          
    atomic_t count;
    //rwlock_t lock;
    int umask;
    struct path root, pwd;
};


#define __FD_SETSIZE    1024


typedef struct {
    unsigned long fds_bits [__FD_SETSIZE];
} __kernel_fd_set;



typedef __kernel_fd_set     fd_set;


struct fdtable {                                                                                            
    unsigned int max_fds;
    struct file ** fd;      /* current fd array */ 
    fd_set *close_on_exec;
    fd_set *open_fds;
    struct list_head rcu;
    struct fdtable *next;
};



#define NR_OPEN_DEFAULT 1024


/*  
 * Open file table structure
 */
struct files_struct {
  /*
   * read mostly part
   */
    atomic_t count;
    struct fdtable *fdt;
    struct fdtable fdtab;
  /*
   * written part on a separate cache line in SMP
   */
    //spinlock_t file_lock ____cacheline_aligned_in_smp;
    int next_fd;
    //struct embedded_fd_set close_on_exec_init;
    //struct embedded_fd_set open_fds_init;
    struct file * fd_array[NR_OPEN_DEFAULT];
};


/*
 * "quick string" -- eases parameter passing, but more importantly
 * saves "metadata" about the string (ie length and the hash).
 *
 * hash comes first so it snuggles against d_parent in the
 * dentry.
 */ 
struct qstr {
    unsigned int hash;
    unsigned int len;
    const unsigned char *name;
};


#define DNAME_INLINE_LEN_MIN 1024



/*存放目录(也就是文件的名称)与对应文件进行链接的有关信息， 
每个磁盘文件系统都以自己特有的方式将该类信息放在磁盘上*/  
struct dentry {  
	atomic_t d_count;/*目录项引用对象*/  
	unsigned int d_flags;/*目录项高速缓存标志*/ 	 /* protected by d_lock */	
	spinlock_t d_lock;/*保护目录项对象的自旋锁*/	   /* per dentry lock */  
	int d_mounted;/*对于目录而言，用于记录安装该目录项的文件系统数的计数器*/  
	struct inode *d_inode;/*与文件名关联的索引节点*/	   /* Where the name belongs to - NULL is negative */  //如果dentry对象是为一个不存在的文件名建立的，则d_inode为NULL指针。这有助于加速查找不存在的文件名，通常情况下，这与查找实际存在的文件名同样耗时。
	/* 
	 * The next three fields are touched by __d_lookup.  Place them here 
	 * so they all fit in a cache line. 
	 */  
	struct hlist_node d_hash;/*指向散列表项链表的指针*/    /* lookup hash list */  
	struct dentry *d_parent;	/* parent directory */	
	struct qstr d_name;/*文件名,文件名较长时，需要动态分配空间，放在这里*/	//qstr是一个内核字符串的包装器，他存储了实际的char*字符串以及字符串长度和散列值，是的更容积处理查找工作。这里并不存储绝对路径，只有路径的最有一个分量，例如对/usr/bin/emacs只存储emacs，因为上述链表结构已经映射了目录结构。
  
	struct list_head d_lru;/*用于未使用目录项链表的指针*/		 /* LRU list */  
	/* 
	 * d_child and d_rcu can share memory 
	 */  
	union {  
		struct list_head d_child;/*对于目录而言，用于统一父目录中的目录项链表的指针*/	/* child of parent list */	
		struct list_head d_rcu;	
	} d_u;	
	struct list_head d_subdirs;/*对目录，子目录项链表的指针*/	 /* our children */  
	struct list_head d_alias;/*用于与同意索引节点相关的目录项链表的指针*/	/* inode alias list */	
	unsigned long d_time;		/* used by d_revalidate */	
	const struct dentry_operations *d_op;  
	struct super_block *d_sb;	/* The root of the dentry tree */  
	void *d_fsdata;/*依赖于文件系统的数据*/ 		  /* fs-specific data */  
  
	unsigned char d_iname[DNAME_INLINE_LEN_MIN];	/*存放短文件的空间,文件名长度小于36时放在这里*//* small names */  //如果文件名只有少量字符组成，则保存在d_iname中，而不是dname中，以加速访问。短文件名的长度上限由DNAME_INLINE_NAME_LEN指定，最多不超过16个字符。
};	



#define MAX_NESTED_LINKS 1024


struct nameidata {                                                                                          
    struct path path;
    struct qstr last;
    unsigned int    flags;
    int     last_type;
    unsigned    depth;
    char *saved_names[MAX_NESTED_LINKS + 1];

    /* Intent data */
    union {
        //struct open_intent open;
    } intent;
}; 



struct dentry_operations {
    int (*d_revalidate)(struct dentry *, struct nameidata *);
    int (*d_hash) (struct dentry *, struct qstr *);//计算hash值
    int (*d_compare) (struct dentry *, struct qstr *, struct qstr *);//比较两个dentry对象的文件名。
    int (*d_delete)(struct dentry *);//在最后一个引用已经移除（d_count到达0时）后，将调用d_delete。
    void (*d_release)(struct dentry *);
    void (*d_iput)(struct dentry *, struct inode *);//从一个不再使用的dentry对象中释放inode（在默认情况下，将inode的使用计数器减1，计数器到达0时，将inode从各种链表中移除）。
    char *(*d_dname)(struct dentry *, char *, int);
};




struct mnt_namespace {                                                                                      
    atomic_t        count;
    struct vfsmount *   root;
    struct list_head    list;
    wait_queue_head_t poll;
    int event;
};


/*
 *  Berkeley style UIO structures   -   Alan Cox 1994.
 *
 *      This program is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU General Public License
 *      as published by the Free Software Foundation; either version
 *      2 of the License, or (at your option) any later version.
 */

struct iovec                                                                                                
{
    void *iov_base;  /* BSD uses caddr_t (1003.1g requires void *) */
    u32 iov_len; /* Must be size_t (1003.1g) */
};


/* is there a better place to document function pointer methods? */
/**
 * ki_retry -   iocb forward progress callback
 * @kiocb:  The kiocb struct to advance by performing an operation.
 *
 * This callback is called when the AIO core wants a given AIO operation
 * to make forward progress.  The kiocb argument describes the operation
 * that is to be performed.  As the operation proceeds, perhaps partially,
 * ki_retry is expected to update the kiocb with progress made.  Typically
 * ki_retry is set in the AIO core and it itself calls file_operations
 * helpers.
 *
 * ki_retry's return value determines when the AIO operation is completed
 * and an event is generated in the AIO event ring.  Except the special
 * return values described below, the value that is returned from ki_retry
 * is transferred directly into the completion ring as the operation's
 * resulting status.  Once this has happened ki_retry *MUST NOT* reference
 * the kiocb pointer again.
 *
 * If ki_retry returns -EIOCBQUEUED it has made a promise that aio_complete()
 * will be called on the kiocb pointer in the future.  The AIO core will
 * not ask the method again -- ki_retry must ensure forward progress.
 * aio_complete() must be called once and only once in the future, multiple
 * calls may result in undefined behaviour.
 *
 * If ki_retry returns -EIOCBRETRY it has made a promise that kick_iocb()
 * will be called on the kiocb pointer in the future.  This may happen
 * through generic helpers that associate kiocb->ki_wait with a wait
 * queue head that ki_retry uses via current->io_wait.  It can also happen
 * with custom tracking and manual calls to kick_iocb(), though that is
 * discouraged.  In either case, kick_iocb() must be called once and only
 * once.  ki_retry must ensure forward progress, the AIO core will wait
 * indefinitely for kick_iocb() to be called.
 */
struct kiocb {
    struct list_head    ki_run_list;
    unsigned long       ki_flags;
    int         ki_users;
    unsigned        ki_key;     /* id of this request */

    struct file     *ki_filp;
    //struct kioctx       *ki_ctx;    /* may be NULL for sync ops */
    //int         (*ki_cancel)(struct kiocb *, struct io_event *);
    ssize_t         (*ki_retry)(struct kiocb *);
    void            (*ki_dtor)(struct kiocb *);

    union {
        void      *user;
        //struct task_struct  *tsk;
    } ki_obj;                                                                       

    u64           ki_user_data;   /* user's data for completion */
    //wait_queue_t        ki_wait;
    loff_t          ki_pos;

    void            *private;
    /* State that we remember to be able to restart/retry  */
    unsigned short      ki_opcode;
    size_t          ki_nbytes;  /* copy of iocb->aio_nbytes */
    char            *ki_buf; /* remaining iocb->aio_buf */
    size_t          ki_left;    /* remaining bytes */
    struct iovec        ki_inline_vec;  /* inline vector */
    struct iovec        *ki_iovec;
    unsigned long       ki_nr_segs;
    unsigned long       ki_cur_seg;

    struct list_head    ki_list;    /* the aio core uses this
                         * for cancellation */

    /*
     * If the aio_resfd field of the userspace iocb is not zero,
     * this is the underlying file* to deliver event to.
     */
    struct file     *ki_eventfd;
};



/*
 * The POSIX file lock owner is determined by
 * the "struct files_struct" in the thread group
 * (or NULL for no owner - BSD locks).
 *
 * Lockd stuffs a "host" pointer into this.
 */
typedef struct files_struct *fl_owner_t;



/*
 * This is the "filldir" function type, used by readdir() to let
 * the kernel specify what kind of dirent layout it wants to have.
 * This allows the kernel to read directories into kernel space or
 * to have different dirent layouts depending on the binary type.
 */
typedef int (*filldir_t)(void *, const char *, int, loff_t, u64, unsigned);


/*
 * NOTE:
 * read, write, poll, fsync, readv, writev, unlocked_ioctl and compat_ioctl
 * can be called without the big kernel lock held in all filesystems.
 */ 
struct file_operations {
    loff_t (*llseek) (struct file *, loff_t, int);
    ssize_t (*read) (struct file *, char *, size_t, loff_t *);
    ssize_t (*write) (struct file *, const char *, size_t, loff_t *);
    ssize_t (*aio_read) (struct kiocb *, const struct iovec *, unsigned long, loff_t);
    ssize_t (*aio_write) (struct kiocb *, const struct iovec *, unsigned long, loff_t);
    int (*readdir) (struct file *, void *, filldir_t);
    unsigned int (*poll) (struct file *, struct poll_table_struct *);
    int (*ioctl) (struct inode *, struct file *, unsigned int, unsigned long);
    long (*unlocked_ioctl) (struct file *, unsigned int, unsigned long);
    long (*compat_ioctl) (struct file *, unsigned int, unsigned long);
    int (*mmap) (struct file *, struct vm_area_struct *);
    int (*open) (struct inode *, struct file *);
    int (*flush) (struct file *, fl_owner_t id);
    int (*release) (struct inode *, struct file *);
    int (*fsync) (struct file *, struct dentry *, int datasync);
    int (*aio_fsync) (struct kiocb *, int datasync);
    int (*fasync) (int, struct file *, int);
    int (*lock) (struct file *, int, struct file_lock *);
    ssize_t (*sendpage) (struct file *, struct page *, int, size_t, loff_t *, int);
    unsigned long (*get_unmapped_area)(struct file *, unsigned long, unsigned long, unsigned long, unsigned long);
    int (*check_flags)(int);
    int (*dir_notify)(struct file *filp, unsigned long arg);
    int (*flock) (struct file *, int, struct file_lock *);
    ssize_t (*splice_write)(struct pipe_inode_info *, struct file *, loff_t *, size_t, unsigned int);
    ssize_t (*splice_read)(struct file *, loff_t *, struct pipe_inode_info *, size_t, unsigned int);
    int (*setlease)(struct file *, long, struct file_lock **);
};



struct vfsmount {
    struct list_head mnt_hash;
    struct vfsmount *mnt_parent;    /* fs we are mounted on */
    struct dentry *mnt_mountpoint;  /* dentry of mountpoint */
    struct dentry *mnt_root;    /* root of the mounted tree */
    struct super_block *mnt_sb; /* pointer to superblock */
    struct list_head mnt_mounts;    /* list of children, anchored here */
    struct list_head mnt_child; /* and going through their mnt_child */
    int mnt_flags;
    /* 4 bytes hole on 64bits arches */
    const char *mnt_devname;    /* Name of device e.g. /dev/dsk/hda1 */
    struct list_head mnt_list;
    struct list_head mnt_expire;    /* link in fs-specific expiry list */
    struct list_head mnt_share; /* circular list of shared mounts */
    struct list_head mnt_slave_list;/* list of slave mounts */
    struct list_head mnt_slave; /* slave list entry */
    struct vfsmount *mnt_master;    /* slave is on master->mnt_slave_list */
    struct mnt_namespace *mnt_ns;   /* containing namespace */
    int mnt_id;         /* mount identifier */
    int mnt_group_id;       /* peer group identifier */
    /*
     * We put mnt_count & mnt_expiry_mark at the end of struct vfsmount
     * to let these frequently modified fields in a separate cache line
     * (so that reads of mnt_flags wont ping-pong on SMP machines)
     */
    atomic_t mnt_count;
    int mnt_expiry_mark;        /* true if marked for expiry */
    int mnt_pinned;
    int mnt_ghosts;
    /*
     * This value is not stable unless all of the mnt_writers[] spinlocks
     * are held, and all mnt_writer[]s on this mount have 0 as their ->count
     */
    atomic_t __mnt_writers;
}; 






struct super_block {  
	struct list_head	s_list;/*指向超级块链表的指针*//* Keep this first */  
	dev_t			s_dev;/*设备标识符*//* search index; _not_ kdev_t */  
	unsigned long		s_blocksize;/*以字节为单位的块大小*/  
	unsigned char		s_blocksize_bits;/*以位为单位的块大小*/  
	unsigned char		s_dirt;/*修改标志*/  
	loff_t			s_maxbytes;/*文件的最长长度*/  /* Max file size */	
	struct file_system_type *s_type;/*文件系统类型*/  
	const struct super_operations	*s_op;/*超级快方法*/  
	//const struct dquot_operations	*dq_op;/*磁盘限额方法*/  
	//const struct quotactl_ops	*s_qcop;/*磁盘限额管理方法*/  
	//const struct export_operations *s_export_op;/*网络文件系统使用的输出方法*/	
	unsigned long		s_flags;/*安装标识*/  
	unsigned long		s_magic;/*文件系统的魔术*/	
	struct dentry		*s_root;/*文件系统根目录的目录项对象*/	
	//struct rw_semaphore s_umount;/*卸载用的信号量*/  
	//struct mutex		s_lock;/*超级块信号量*/  
	int 		s_count;/*引用计数*/  
	int 		s_need_sync;/*表示对超级快的索引节点进行同步标志*/	
	atomic_t		s_active;/*次级引用计数*/  
#ifdef CONFIG_SECURITY	
	void					*s_security;/*指向超级安全数据结构的指针*/	
#endif	
	//struct xattr_handler	**s_xattr;/*执行超级快扩展数据结构的指针*/	
  
	struct list_head	s_inodes;/*所有索引节点的链表*/  /* all inodes */  
	struct hlist_head	s_anon;/*用于处理网络文件系统的匿名目录项的链表*/	   /*  
anonymous dentries for (nfs) exporting */  
	struct list_head	s_files;/*文件对象的链表*/	
	/* s_dentry_lru and s_nr_dentry_unused are protected by dcache_lock */	
	struct list_head	s_dentry_lru;/* /* unused dentry lru */  
	int 		s_nr_dentry_unused; /* # of dentry on lru */  
  
	struct block_device *s_bdev;/*指向块设备驱动程序描述符的指针*/	
	struct backing_dev_info *s_bdi;/* */  
	//struct mtd_info 	*s_mtd;/**/  
	struct list_head	s_instances;/*用于指定文件系统类型的超级快对象链表指针*/  
	//struct quota_info	s_dquot;/*磁盘限额的描述符*/	/* Diskquota specific options  */	
  
	int 		s_frozen;/*冻结文件系统时使用的标志*/  
	wait_queue_head_t	s_wait_unfrozen;/*进程挂起的等待队列，直到文件系统被冻结*/	
  
	char s_id[32];/*包含超级快的块设备名称*/			   /* Informational name */  
  
	void			*s_fs_info;/*指向特定文件系统的超级快信息的指针*/	 /* Filesystem private info */  
	//fmode_t 		s_mode;/**/  
  
	/* 
	 * The next field is for VFS *only*. No filesystems have any business 
	 * even looking at it. You had been warned. 
	 */  
	//struct mutex s_vfs_rename_mutex;/*当VFS通过目录命名文件时使用的互斥变量*/  
  
	/* Granularity of c/m/atime in ns. 
	   Cannot be worse than a second */  
	u32 	   s_time_gran;/*时间戳的粒度*/  
  
	/* 
	 * Filesystem subtype.	If non-empty the filesystem type field 
	 * in /proc/mounts will be "type.subtype" 
	 */  
	char *s_subtype;/**/  
  
	/* 
	 * Saved mount options for lazy filesystems using 
	 * generic_show_options() 
	 */  
	char *s_options;  
};	


struct kstatfs {                                                                                            
    long f_type;
    long f_bsize;
    u64 f_blocks;
    u64 f_bfree;
    u64 f_bavail;
    u64 f_files;
    u64 f_ffree;
    //__kernel_fsid_t f_fsid;
    long f_namelen;
    long f_frsize;      
    long f_spare[5];
};


struct super_operations {
    struct inode *(*alloc_inode)(struct super_block *sb);
    void (*destroy_inode)(struct inode *);

    void (*dirty_inode) (struct inode *);
    int (*write_inode) (struct inode *, int);
    void (*drop_inode) (struct inode *);
    void (*delete_inode) (struct inode *);
    void (*put_super) (struct super_block *);
    void (*write_super) (struct super_block *);
    int (*sync_fs)(struct super_block *sb, int wait);
    void (*write_super_lockfs) (struct super_block *);
    void (*unlockfs) (struct super_block *);
    int (*statfs) (struct dentry *, struct kstatfs *);
    int (*remount_fs) (struct super_block *, int *, char *);
    void (*clear_inode) (struct inode *);
    void (*umount_begin) (struct super_block *);

    //int (*show_options)(struct seq_file *, struct vfsmount *);
    //int (*show_stats)(struct seq_file *, struct vfsmount *);
#ifdef CONFIG_QUOTA
    ssize_t (*quota_read)(struct super_block *, int, char *, size_t, loff_t);
    ssize_t (*quota_write)(struct super_block *, int, const char *, size_t, loff_t);
#endif
};



struct address_space {
    struct inode        *host;      /* owner: inode, block_device */
    struct rxt_node  page_tree;  /* radix tree of all pages */
    spinlock_t      tree_lock;  /* and lock protecting it */
    unsigned int        i_mmap_writable;/* count VM_SHARED mappings */
    struct prio_tree_root   i_mmap;     /* tree of private and shared mappings */
    struct list_head    i_mmap_nonlinear;/*list VM_NONLINEAR mappings */
    spinlock_t      i_mmap_lock;    /* protect tree, count, list */
    unsigned int        truncate_count; /* Cover race condition with truncate */
    unsigned long       nrpages;    /* number of total pages */
    pgoff_t         writeback_index;/* writeback starts here */
    const struct address_space_operations *a_ops;   /* methods */
    unsigned long       flags;      /* error bits/gfp mask */
    struct backing_dev_info *backing_dev_info; /* device readahead, etc */
    spinlock_t      private_lock;   /* for use by the address_space */
    struct list_head    private_list;   /* ditto */
    struct address_space    *assoc_mapping; /* ditto */
} __attribute__((aligned(sizeof(long))));




/*索引节点，要访问一个文件时，一定要通过他的索引才能知道 
这个文件是什么类型的文件、是怎么组织的、文件中存储这 
多少数据、这些数据在什么地方以及其下层的驱动程序在哪儿等 
必要的信息*/  
struct inode {	
	struct hlist_node	i_hash;/*用于散列链表的指针*/ //fs/inode.c中的hash函数用于计算散列和。他将inode编号和超级块对象的地址合并为一个唯一的编号，保证位于散列表已经分配的下标范围内。碰撞照例通过移除链表解决。Inode的成员i_hash用于管理溢出链表。
	struct list_head	i_list; //每个inode都有一个i_list成员，可以将inode存储在一个链表中。根据inode的状态，他可能有3种主要的情况。（1）inode存在于内存中，未关联到任何文件，也不处于活动使用状态（2）inode结构在内存中，正在由一个或多个进程使用，通常表示一个文件。两个计数器（i_count和i_nlink）的值都必须大于0，文件内容和inode元数据都与底层块设备上的信息相同。也就是说，从上一次与存储介质同步以来，该inode没有改变过。（3）inode处于活动使用状态。其数据内容已经改变，与存储介质的内容不同。这种状态的inode叫做脏的。
	struct list_head	i_sb_list;/*用于超级快的索引节点链表的指针*/  //除了散列表之外，inode还通过一个特定于超级块的链表维护，表头是super_block->s_inodes。I_sb_list用作链表元素。但超级块管理了更多的inode链表，与i_sb_list所在的链表是独立的。如果一个inode是脏的，即内容已经被修改，则列入脏链表，表头为super_block->s_dirty，链表元素是i_list.这样做有下列函数：在写回数据时候（数据回写通常也称之为同步）不需要扫描系统所有的inode，考虑脏链表上所有的inode就足够了。另外两个链表（表头为super_block->s_io和super_block->s_more_io）使用同样的链表元素i_list。这两个链表包含的是已经选中向磁盘回写的inode，但正在等待回写进行。
	struct list_head	i_dentry;/*引用索引节点的目录项对象链表头*/  
	unsigned long		i_ino;/*索引节点号*/  
	atomic_t		i_count;/*引用计数器*/	
	unsigned int		i_nlink;/*硬链接数目*/	
	//uid_t			i_uid;/*所有者标识符*/	
	//gid_t			i_gid;/*主标识符*/	
	dev_t			i_rdev;/*实设备标识符*/  
	u64 		i_version;/*版本号，每次使用后自动增加*/  
	loff_t			i_size;/*文件的字节数*/  
#ifdef __NEED_I_SIZE_ORDERED  
	seqcount_t		i_size_seqcount;  
#endif	
	//struct timespec 	i_atime;/*上次访问文件的时间*/	
	//struct timespec 	i_mtime;/*上次写文件的时间*/  
	//struct timespec 	i_ctime;/*上次修改索引节点的时间*/	
	u32		i_blocks;/*文件的块数*/  
	u32		i_blkbits;/*块的位数*/	
	u16			i_bytes;/*文件最后一个块的字节数*/	
	u32 		i_mode;/**/  
	spinlock_t		i_lock;/*保护索引节点一些字段的子璇锁*/   /* i_blocks, i_bytes,  
maybe i_size */  
	//struct mutex		i_mutex;/**/  
	//struct rw_semaphore i_alloc_sem;/*在直接IO 文件操作中避免出现竞争条件的读写信号量*/  
	const struct inode_operations	*i_op;/*索引节点的操作*/  
	const struct file_operations	*i_fop;/*缺省文件操作*/   /* former ->i_op-> default_file_ops */  
	struct super_block	*i_sb;/*指向超级快对象的指针*/	
	//struct file_lock	*i_flock;/*指向文件锁链表的指针*/  
	struct address_space	*i_mapping;/*指向address_space对象的指针?*/  
	struct address_space	i_data;/*文件的address_space对象*/	
#ifdef CONFIG_QUOTA  
	struct dquot		*i_dquot[MAXQUOTAS];/*索引节点磁盘限额*/  
#endif	
	struct list_head	i_devices;/*用于具体的字符或块设备索引节点链表指针*/  
	union {  
		//struct pipe_inode_info	*i_pipe;/*如果文件是一个管道，则使用他8、*/  
		struct block_device *i_bdev;/*指向块设备驱动的指针*/  
		//struct cdev 	*i_cdev;/*指向字符设备驱动的指针*/	
	};	
  
	u32			i_generation;/*索引节点版本号*/  
  
#ifdef CONFIG_FSNOTIFY	
	u32			i_fsnotify_mask;/*目录通知事件的位掩码*//* all events this inode  
cares about */	
	struct hlist_head	i_fsnotify_mark_entries; /* fsnotify mark entries */  
#endif	
  
#ifdef CONFIG_INOTIFY  
	struct list_head	inotify_watches; /* watches on this inode */  
	struct mutex		inotify_mutex;	/* protects the watches list */  
#endif	
  
	unsigned long		i_state;/*索引节点的状态标志*/	
	unsigned long		dirtied_when;/*索引节点的弄脏标志*/  /* jiffies of first  
dirtying */  
  
	unsigned int		i_flags;/*文件系统的安装标志*/	
  
	atomic_t		i_writecount;/*用于写进程的引用计数*/  
#ifdef CONFIG_SECURITY	
	void			*i_security;/*指向索引节点安全结构的指针*/	
#endif	
#ifdef CONFIG_FS_POSIX_ACL	
	struct posix_acl	*i_acl;/**/  
	struct posix_acl	*i_default_acl;  
#endif	
	void			*i_private; /* fs or device private pointer */	
};	


struct inode_operations {
    int (*create) (struct inode *,struct dentry *,int, struct nameidata *);
    struct dentry * (*lookup) (struct inode *,struct dentry *, struct nameidata *);  //根据文件系统对象的名称（表示为字符串）查找其inode实例。
    int (*link) (struct dentry *,struct inode *,struct dentry *);//用于删除文件。但根据上文的描述，如果硬链接的引用计数器表示该inode仍然被多个文件使用，则不会执行删除操作。
    int (*unlink) (struct inode *,struct dentry *);
    int (*symlink) (struct inode *,struct dentry *,const char *);
    int (*mkdir) (struct inode *,struct dentry *,int);
    int (*rmdir) (struct inode *,struct dentry *);
    int (*mknod) (struct inode *,struct dentry *,int,dev_t);
    int (*rename) (struct inode *, struct dentry *, struct inode *, struct dentry *);
    int (*readlink) (struct dentry *, char *,int);
    void * (*follow_link) (struct dentry *, struct nameidata *);//根据符号链接查找目标文件的inode。因为符号链接可能是跨文件系统边界的，该例程的实现通常非常短，实际工作很快委托给一般的VFS例程完成。
    void (*put_link) (struct dentry *, struct nameidata *, void *);
    void (*truncate) (struct inode *);//修改指定inode的长度。该函数只接受一个参数，即所处理的inode的数据结构。在调用该函数之前，必须将新的文件长度手工设置到inode结构的i_size成员。
    int (*permission) (struct inode *, int);
//xattr函数建立、读取、删除文件的扩展属性，经典的UNIX模型不支持这些属性。例如，可使用这些属性实现访问控制表（access control list ACL）。
    int (*setattr) (struct dentry *, struct iattr *);
    int (*getattr) (struct vfsmount *mnt, struct dentry *, struct kstat *);
    int (*setxattr) (struct dentry *, const char *,const void *,size_t,int);
    ssize_t (*getxattr) (struct dentry *, const char *, void *, size_t);
    ssize_t (*listxattr) (struct dentry *, char *, size_t);
    int (*removexattr) (struct dentry *, const char *);
    void (*truncate_range)(struct inode *, loff_t, loff_t);//用于截断一个范围内的块（即在文件中穿孔），但该操作当前只有共享内存文件系统支持。
    long (*fallocate)(struct inode *inode, int mode, loff_t offset, loff_t len);//用于对文件预先分配空间，在一些情况下可以提高性能。但只有很新的文件系统（如reiserfs或ext4）才支持该操作。
};



/*存放打开文件与进程之间进行交互的有关信息。这类信息仅当进程访问 
文件期间存放在内核内存中。如文件的读取或写位置，以及 
读写同步等操作*/  
struct file {  
	/* 
	 * fu_list becomes invalid after file_free is called and queued via 
	 * fu_rcuhead for RCU freeing 
	 */  
	union {  
		struct list_head	fu_list;  //每个超级块都提供了一个s_list成员作为表头，以建立file对象的链表，链表元素是file->f_list。该链表包含该超级块表示的文件系统的所有打开文件。例如，在以读、写模式装载的文件系统以只读模式重新装载时，会扫描该链表。当然，如果仍然有按写模式打开的文件，是无法重新装载的，因而内核需要检查该链表来确认。
		struct list_head 	fu_rcuhead;  
	} f_u;	
	struct path 	f_path;  //封装了（1）文件名和inode之间的关联（2）文件所在文件系统之间的有关信息
#define f_dentry	f_path.dentry  
#define f_vfsmnt	f_path.mnt	
	const struct file_operations	*f_op;	//指定了文件操作调用的各个函数
	spinlock_t		f_lock;  /* f_ep_links, f_flags, no IRQ */	
	atomic_t		f_count;  
	unsigned int		f_flags;  
	u32 		f_mode;/*打开文件的模式*/  
	loff_t			f_pos;/*文件的读写位置*/  
	//struct fown_struct	f_owner;  //f_owner包含了处理该文件的进程有关的信息（因而也确定了SIGIO信号发送的目标pid，以实现异步IO）。
	//const struct cred	*f_cred;  
	//struct file_ra_state	f_ra; //预读特征保存在f_ra。这些值指定了在实际请求文件数据之前，是否预读文件数据、如果预读（预读可以提供系统系能）、 
  
	u64 		f_version;	//由文件系统使用，以检查一个file实例是否仍然与相关的inode内容兼容。这对于确保已缓存对象的一致性很重要。
#ifdef CONFIG_SECURITY	
	void			*f_security;  
#endif	
	/* needed for tty driver, and maybe others */  
	void			*private_data;	
  
#ifdef CONFIG_EPOLL  
	/* Used by fs/eventpoll.c to link all the hooks to this file */  
	struct list_head	f_ep_links;  
#endif /* #ifdef CONFIG_EPOLL */  
	struct address_space	*f_mapping;/*指向node节点的页面高速缓存*/  //指向属于文件相关的inode实例的地址空间映射。通常他设置为inode->i_mapping，但文件系统或其他内核子系统可能会修改它。
#ifdef CONFIG_DEBUG_WRITECOUNT	
	unsigned long f_mnt_write_state;  
#endif	
};	


struct file_system_type {  
	const char *name;/*文件系统的名称*/  
	int fs_flags;/*文件系统类型标志*/  
	int (*get_sb) (struct file_system_type *, int,	
			   const char *, void *, struct vfsmount *);/*读超级快的方法*/	
	void (*kill_sb) (struct super_block *);/*删除超级块的方法*/  
	//struct module *owner;/*指向实现文件系统的模块指针*/  
	struct file_system_type * next;/*指向文件系统类型链表中下一个元素的指针*/  
	struct list_head fs_supers;/*具有相同文件系统类型的超级块对象链表头*/  
  
	//struct lock_class_key s_lock_key;  
	//struct lock_class_key s_umount_key;  
  
	//struct lock_class_key i_lock_key;  
	//struct lock_class_key i_mutex_key;	
	//struct lock_class_key i_mutex_dir_key;	
	//struct lock_class_key i_alloc_sem_key;	
};	



/*
 * Handling of filesystem drivers list.
 * Rules:
 *  Inclusion to/removals from/scanning of list are protected by spinlock.
 *  During the unload module must call unregister_filesystem().
 *  We can access the fields of list element if:
 *      1) spinlock is held or
 *      2) we hold the reference to the module.
 *  The latter can be guaranteed by call of try_module_get(); if it
 *  returned 0 we must skip the element, otherwise we got the reference.
 *  Once the reference is obtained we can drop the spinlock.
 */

struct file_system_type *file_systems;

struct vfsmount *vfsmntlist;

LIST_HEAD(super_blocks);



int vfs_init();




#endif
