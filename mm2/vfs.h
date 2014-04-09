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



/*���Ŀ¼(Ҳ�����ļ�������)���Ӧ�ļ��������ӵ��й���Ϣ�� 
ÿ�������ļ�ϵͳ�����Լ����еķ�ʽ��������Ϣ���ڴ�����*/  
struct dentry {  
	atomic_t d_count;/*Ŀ¼�����ö���*/  
	unsigned int d_flags;/*Ŀ¼����ٻ����־*/ 	 /* protected by d_lock */	
	spinlock_t d_lock;/*����Ŀ¼������������*/	   /* per dentry lock */  
	int d_mounted;/*����Ŀ¼���ԣ����ڼ�¼��װ��Ŀ¼����ļ�ϵͳ���ļ�����*/  
	struct inode *d_inode;/*���ļ��������������ڵ�*/	   /* Where the name belongs to - NULL is negative */  //���dentry������Ϊһ�������ڵ��ļ��������ģ���d_inodeΪNULLָ�롣�������ڼ��ٲ��Ҳ����ڵ��ļ�����ͨ������£��������ʵ�ʴ��ڵ��ļ���ͬ����ʱ��
	/* 
	 * The next three fields are touched by __d_lookup.  Place them here 
	 * so they all fit in a cache line. 
	 */  
	struct hlist_node d_hash;/*ָ��ɢ�б��������ָ��*/    /* lookup hash list */  
	struct dentry *d_parent;	/* parent directory */	
	struct qstr d_name;/*�ļ���,�ļ����ϳ�ʱ����Ҫ��̬����ռ䣬��������*/	//qstr��һ���ں��ַ����İ�װ�������洢��ʵ�ʵ�char*�ַ����Լ��ַ������Ⱥ�ɢ��ֵ���ǵĸ��ݻ�������ҹ��������ﲢ���洢����·����ֻ��·��������һ�������������/usr/bin/emacsֻ�洢emacs����Ϊ��������ṹ�Ѿ�ӳ����Ŀ¼�ṹ��
  
	struct list_head d_lru;/*����δʹ��Ŀ¼�������ָ��*/		 /* LRU list */  
	/* 
	 * d_child and d_rcu can share memory 
	 */  
	union {  
		struct list_head d_child;/*����Ŀ¼���ԣ�����ͳһ��Ŀ¼�е�Ŀ¼�������ָ��*/	/* child of parent list */	
		struct list_head d_rcu;	
	} d_u;	
	struct list_head d_subdirs;/*��Ŀ¼����Ŀ¼�������ָ��*/	 /* our children */  
	struct list_head d_alias;/*������ͬ�������ڵ���ص�Ŀ¼�������ָ��*/	/* inode alias list */	
	unsigned long d_time;		/* used by d_revalidate */	
	const struct dentry_operations *d_op;  
	struct super_block *d_sb;	/* The root of the dentry tree */  
	void *d_fsdata;/*�������ļ�ϵͳ������*/ 		  /* fs-specific data */  
  
	unsigned char d_iname[DNAME_INLINE_LEN_MIN];	/*��Ŷ��ļ��Ŀռ�,�ļ�������С��36ʱ��������*//* small names */  //����ļ���ֻ�������ַ���ɣ��򱣴���d_iname�У�������dname�У��Լ��ٷ��ʡ����ļ����ĳ���������DNAME_INLINE_NAME_LENָ������಻����16���ַ���
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
    int (*d_hash) (struct dentry *, struct qstr *);//����hashֵ
    int (*d_compare) (struct dentry *, struct qstr *, struct qstr *);//�Ƚ�����dentry������ļ�����
    int (*d_delete)(struct dentry *);//�����һ�������Ѿ��Ƴ���d_count����0ʱ���󣬽�����d_delete��
    void (*d_release)(struct dentry *);
    void (*d_iput)(struct dentry *, struct inode *);//��һ������ʹ�õ�dentry�������ͷ�inode����Ĭ������£���inode��ʹ�ü�������1������������0ʱ����inode�Ӹ����������Ƴ�����
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
	struct list_head	s_list;/*ָ�򳬼��������ָ��*//* Keep this first */  
	dev_t			s_dev;/*�豸��ʶ��*//* search index; _not_ kdev_t */  
	unsigned long		s_blocksize;/*���ֽ�Ϊ��λ�Ŀ��С*/  
	unsigned char		s_blocksize_bits;/*��λΪ��λ�Ŀ��С*/  
	unsigned char		s_dirt;/*�޸ı�־*/  
	loff_t			s_maxbytes;/*�ļ��������*/  /* Max file size */	
	struct file_system_type *s_type;/*�ļ�ϵͳ����*/  
	const struct super_operations	*s_op;/*�����췽��*/  
	//const struct dquot_operations	*dq_op;/*�����޶��*/  
	//const struct quotactl_ops	*s_qcop;/*�����޶������*/  
	//const struct export_operations *s_export_op;/*�����ļ�ϵͳʹ�õ��������*/	
	unsigned long		s_flags;/*��װ��ʶ*/  
	unsigned long		s_magic;/*�ļ�ϵͳ��ħ��*/	
	struct dentry		*s_root;/*�ļ�ϵͳ��Ŀ¼��Ŀ¼�����*/	
	//struct rw_semaphore s_umount;/*ж���õ��ź���*/  
	//struct mutex		s_lock;/*�������ź���*/  
	int 		s_count;/*���ü���*/  
	int 		s_need_sync;/*��ʾ�Գ�����������ڵ����ͬ����־*/	
	atomic_t		s_active;/*�μ����ü���*/  
#ifdef CONFIG_SECURITY	
	void					*s_security;/*ָ�򳬼���ȫ���ݽṹ��ָ��*/	
#endif	
	//struct xattr_handler	**s_xattr;/*ִ�г�������չ���ݽṹ��ָ��*/	
  
	struct list_head	s_inodes;/*���������ڵ������*/  /* all inodes */  
	struct hlist_head	s_anon;/*���ڴ��������ļ�ϵͳ������Ŀ¼�������*/	   /*  
anonymous dentries for (nfs) exporting */  
	struct list_head	s_files;/*�ļ����������*/	
	/* s_dentry_lru and s_nr_dentry_unused are protected by dcache_lock */	
	struct list_head	s_dentry_lru;/* /* unused dentry lru */  
	int 		s_nr_dentry_unused; /* # of dentry on lru */  
  
	struct block_device *s_bdev;/*ָ����豸����������������ָ��*/	
	struct backing_dev_info *s_bdi;/* */  
	//struct mtd_info 	*s_mtd;/**/  
	struct list_head	s_instances;/*����ָ���ļ�ϵͳ���͵ĳ������������ָ��*/  
	//struct quota_info	s_dquot;/*�����޶��������*/	/* Diskquota specific options  */	
  
	int 		s_frozen;/*�����ļ�ϵͳʱʹ�õı�־*/  
	wait_queue_head_t	s_wait_unfrozen;/*���̹���ĵȴ����У�ֱ���ļ�ϵͳ������*/	
  
	char s_id[32];/*����������Ŀ��豸����*/			   /* Informational name */  
  
	void			*s_fs_info;/*ָ���ض��ļ�ϵͳ�ĳ�������Ϣ��ָ��*/	 /* Filesystem private info */  
	//fmode_t 		s_mode;/**/  
  
	/* 
	 * The next field is for VFS *only*. No filesystems have any business 
	 * even looking at it. You had been warned. 
	 */  
	//struct mutex s_vfs_rename_mutex;/*��VFSͨ��Ŀ¼�����ļ�ʱʹ�õĻ������*/  
  
	/* Granularity of c/m/atime in ns. 
	   Cannot be worse than a second */  
	u32 	   s_time_gran;/*ʱ���������*/  
  
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




/*�����ڵ㣬Ҫ����һ���ļ�ʱ��һ��Ҫͨ��������������֪�� 
����ļ���ʲô���͵��ļ�������ô��֯�ġ��ļ��д洢�� 
�������ݡ���Щ������ʲô�ط��Լ����²�������������Ķ��� 
��Ҫ����Ϣ*/  
struct inode {	
	struct hlist_node	i_hash;/*����ɢ�������ָ��*/ //fs/inode.c�е�hash�������ڼ���ɢ�к͡�����inode��źͳ��������ĵ�ַ�ϲ�Ϊһ��Ψһ�ı�ţ���֤λ��ɢ�б��Ѿ�������±귶Χ�ڡ���ײ����ͨ���Ƴ���������Inode�ĳ�Աi_hash���ڹ����������
	struct list_head	i_list; //ÿ��inode����һ��i_list��Ա�����Խ�inode�洢��һ�������С�����inode��״̬����������3����Ҫ���������1��inode�������ڴ��У�δ�������κ��ļ���Ҳ�����ڻʹ��״̬��2��inode�ṹ���ڴ��У�������һ����������ʹ�ã�ͨ����ʾһ���ļ���������������i_count��i_nlink����ֵ���������0���ļ����ݺ�inodeԪ���ݶ���ײ���豸�ϵ���Ϣ��ͬ��Ҳ����˵������һ����洢����ͬ����������inodeû�иı������3��inode���ڻʹ��״̬�������������Ѿ��ı䣬��洢���ʵ����ݲ�ͬ������״̬��inode������ġ�
	struct list_head	i_sb_list;/*���ڳ�����������ڵ������ָ��*/  //����ɢ�б�֮�⣬inode��ͨ��һ���ض��ڳ����������ά������ͷ��super_block->s_inodes��I_sb_list��������Ԫ�ء�������������˸����inode������i_sb_list���ڵ������Ƕ����ġ����һ��inode����ģ��������Ѿ����޸ģ���������������ͷΪsuper_block->s_dirty������Ԫ����i_list.�����������к�������д������ʱ�����ݻ�дͨ��Ҳ��֮Ϊͬ��������Ҫɨ��ϵͳ���е�inode�����������������е�inode���㹻�ˡ���������������ͷΪsuper_block->s_io��super_block->s_more_io��ʹ��ͬ��������Ԫ��i_list��������������������Ѿ�ѡ������̻�д��inode�������ڵȴ���д���С�
	struct list_head	i_dentry;/*���������ڵ��Ŀ¼���������ͷ*/  
	unsigned long		i_ino;/*�����ڵ��*/  
	atomic_t		i_count;/*���ü�����*/	
	unsigned int		i_nlink;/*Ӳ������Ŀ*/	
	//uid_t			i_uid;/*�����߱�ʶ��*/	
	//gid_t			i_gid;/*����ʶ��*/	
	dev_t			i_rdev;/*ʵ�豸��ʶ��*/  
	u64 		i_version;/*�汾�ţ�ÿ��ʹ�ú��Զ�����*/  
	loff_t			i_size;/*�ļ����ֽ���*/  
#ifdef __NEED_I_SIZE_ORDERED  
	seqcount_t		i_size_seqcount;  
#endif	
	//struct timespec 	i_atime;/*�ϴη����ļ���ʱ��*/	
	//struct timespec 	i_mtime;/*�ϴ�д�ļ���ʱ��*/  
	//struct timespec 	i_ctime;/*�ϴ��޸������ڵ��ʱ��*/	
	u32		i_blocks;/*�ļ��Ŀ���*/  
	u32		i_blkbits;/*���λ��*/	
	u16			i_bytes;/*�ļ����һ������ֽ���*/	
	u32 		i_mode;/**/  
	spinlock_t		i_lock;/*���������ڵ�һЩ�ֶε������*/   /* i_blocks, i_bytes,  
maybe i_size */  
	//struct mutex		i_mutex;/**/  
	//struct rw_semaphore i_alloc_sem;/*��ֱ��IO �ļ������б�����־��������Ķ�д�ź���*/  
	const struct inode_operations	*i_op;/*�����ڵ�Ĳ���*/  
	const struct file_operations	*i_fop;/*ȱʡ�ļ�����*/   /* former ->i_op-> default_file_ops */  
	struct super_block	*i_sb;/*ָ�򳬼�������ָ��*/	
	//struct file_lock	*i_flock;/*ָ���ļ��������ָ��*/  
	struct address_space	*i_mapping;/*ָ��address_space�����ָ��?*/  
	struct address_space	i_data;/*�ļ���address_space����*/	
#ifdef CONFIG_QUOTA  
	struct dquot		*i_dquot[MAXQUOTAS];/*�����ڵ�����޶�*/  
#endif	
	struct list_head	i_devices;/*���ھ�����ַ�����豸�����ڵ�����ָ��*/  
	union {  
		//struct pipe_inode_info	*i_pipe;/*����ļ���һ���ܵ�����ʹ����8��*/  
		struct block_device *i_bdev;/*ָ����豸������ָ��*/  
		//struct cdev 	*i_cdev;/*ָ���ַ��豸������ָ��*/	
	};	
  
	u32			i_generation;/*�����ڵ�汾��*/  
  
#ifdef CONFIG_FSNOTIFY	
	u32			i_fsnotify_mask;/*Ŀ¼֪ͨ�¼���λ����*//* all events this inode  
cares about */	
	struct hlist_head	i_fsnotify_mark_entries; /* fsnotify mark entries */  
#endif	
  
#ifdef CONFIG_INOTIFY  
	struct list_head	inotify_watches; /* watches on this inode */  
	struct mutex		inotify_mutex;	/* protects the watches list */  
#endif	
  
	unsigned long		i_state;/*�����ڵ��״̬��־*/	
	unsigned long		dirtied_when;/*�����ڵ��Ū���־*/  /* jiffies of first  
dirtying */  
  
	unsigned int		i_flags;/*�ļ�ϵͳ�İ�װ��־*/	
  
	atomic_t		i_writecount;/*����д���̵����ü���*/  
#ifdef CONFIG_SECURITY	
	void			*i_security;/*ָ�������ڵ㰲ȫ�ṹ��ָ��*/	
#endif	
#ifdef CONFIG_FS_POSIX_ACL	
	struct posix_acl	*i_acl;/**/  
	struct posix_acl	*i_default_acl;  
#endif	
	void			*i_private; /* fs or device private pointer */	
};	


struct inode_operations {
    int (*create) (struct inode *,struct dentry *,int, struct nameidata *);
    struct dentry * (*lookup) (struct inode *,struct dentry *, struct nameidata *);  //�����ļ�ϵͳ��������ƣ���ʾΪ�ַ�����������inodeʵ����
    int (*link) (struct dentry *,struct inode *,struct dentry *);//����ɾ���ļ������������ĵ����������Ӳ���ӵ����ü�������ʾ��inode��Ȼ������ļ�ʹ�ã��򲻻�ִ��ɾ��������
    int (*unlink) (struct inode *,struct dentry *);
    int (*symlink) (struct inode *,struct dentry *,const char *);
    int (*mkdir) (struct inode *,struct dentry *,int);
    int (*rmdir) (struct inode *,struct dentry *);
    int (*mknod) (struct inode *,struct dentry *,int,dev_t);
    int (*rename) (struct inode *, struct dentry *, struct inode *, struct dentry *);
    int (*readlink) (struct dentry *, char *,int);
    void * (*follow_link) (struct dentry *, struct nameidata *);//���ݷ������Ӳ���Ŀ���ļ���inode����Ϊ�������ӿ����ǿ��ļ�ϵͳ�߽�ģ������̵�ʵ��ͨ���ǳ��̣�ʵ�ʹ����ܿ�ί�и�һ���VFS������ɡ�
    void (*put_link) (struct dentry *, struct nameidata *, void *);
    void (*truncate) (struct inode *);//�޸�ָ��inode�ĳ��ȡ��ú���ֻ����һ�����������������inode�����ݽṹ���ڵ��øú���֮ǰ�����뽫�µ��ļ������ֹ����õ�inode�ṹ��i_size��Ա��
    int (*permission) (struct inode *, int);
//xattr������������ȡ��ɾ���ļ�����չ���ԣ������UNIXģ�Ͳ�֧����Щ���ԡ����磬��ʹ����Щ����ʵ�ַ��ʿ��Ʊ�access control list ACL����
    int (*setattr) (struct dentry *, struct iattr *);
    int (*getattr) (struct vfsmount *mnt, struct dentry *, struct kstat *);
    int (*setxattr) (struct dentry *, const char *,const void *,size_t,int);
    ssize_t (*getxattr) (struct dentry *, const char *, void *, size_t);
    ssize_t (*listxattr) (struct dentry *, char *, size_t);
    int (*removexattr) (struct dentry *, const char *);
    void (*truncate_range)(struct inode *, loff_t, loff_t);//���ڽض�һ����Χ�ڵĿ飨�����ļ��д��ף������ò�����ǰֻ�й����ڴ��ļ�ϵͳ֧�֡�
    long (*fallocate)(struct inode *inode, int mode, loff_t offset, loff_t len);//���ڶ��ļ�Ԥ�ȷ���ռ䣬��һЩ����¿���������ܡ���ֻ�к��µ��ļ�ϵͳ����reiserfs��ext4����֧�ָò�����
};



/*��Ŵ��ļ������֮����н������й���Ϣ��������Ϣ�������̷��� 
�ļ��ڼ������ں��ڴ��С����ļ��Ķ�ȡ��дλ�ã��Լ� 
��дͬ���Ȳ���*/  
struct file {  
	/* 
	 * fu_list becomes invalid after file_free is called and queued via 
	 * fu_rcuhead for RCU freeing 
	 */  
	union {  
		struct list_head	fu_list;  //ÿ�������鶼�ṩ��һ��s_list��Ա��Ϊ��ͷ���Խ���file�������������Ԫ����file->f_list������������ó������ʾ���ļ�ϵͳ�����д��ļ������磬���Զ���дģʽװ�ص��ļ�ϵͳ��ֻ��ģʽ����װ��ʱ����ɨ���������Ȼ�������Ȼ�а�дģʽ�򿪵��ļ������޷�����װ�صģ�����ں���Ҫ����������ȷ�ϡ�
		struct list_head 	fu_rcuhead;  
	} f_u;	
	struct path 	f_path;  //��װ�ˣ�1���ļ�����inode֮��Ĺ�����2���ļ������ļ�ϵͳ֮����й���Ϣ
#define f_dentry	f_path.dentry  
#define f_vfsmnt	f_path.mnt	
	const struct file_operations	*f_op;	//ָ�����ļ��������õĸ�������
	spinlock_t		f_lock;  /* f_ep_links, f_flags, no IRQ */	
	atomic_t		f_count;  
	unsigned int		f_flags;  
	u32 		f_mode;/*���ļ���ģʽ*/  
	loff_t			f_pos;/*�ļ��Ķ�дλ��*/  
	//struct fown_struct	f_owner;  //f_owner�����˴�����ļ��Ľ����йص���Ϣ�����Ҳȷ����SIGIO�źŷ��͵�Ŀ��pid����ʵ���첽IO����
	//const struct cred	*f_cred;  
	//struct file_ra_state	f_ra; //Ԥ������������f_ra����Щֵָ������ʵ�������ļ�����֮ǰ���Ƿ�Ԥ���ļ����ݡ����Ԥ����Ԥ�������ṩϵͳϵ�ܣ��� 
  
	u64 		f_version;	//���ļ�ϵͳʹ�ã��Լ��һ��fileʵ���Ƿ���Ȼ����ص�inode���ݼ��ݡ������ȷ���ѻ�������һ���Ժ���Ҫ��
#ifdef CONFIG_SECURITY	
	void			*f_security;  
#endif	
	/* needed for tty driver, and maybe others */  
	void			*private_data;	
  
#ifdef CONFIG_EPOLL  
	/* Used by fs/eventpoll.c to link all the hooks to this file */  
	struct list_head	f_ep_links;  
#endif /* #ifdef CONFIG_EPOLL */  
	struct address_space	*f_mapping;/*ָ��node�ڵ��ҳ����ٻ���*/  //ָ�������ļ���ص�inodeʵ���ĵ�ַ�ռ�ӳ�䡣ͨ��������Ϊinode->i_mapping�����ļ�ϵͳ�������ں���ϵͳ���ܻ��޸�����
#ifdef CONFIG_DEBUG_WRITECOUNT	
	unsigned long f_mnt_write_state;  
#endif	
};	


struct file_system_type {  
	const char *name;/*�ļ�ϵͳ������*/  
	int fs_flags;/*�ļ�ϵͳ���ͱ�־*/  
	int (*get_sb) (struct file_system_type *, int,	
			   const char *, void *, struct vfsmount *);/*��������ķ���*/	
	void (*kill_sb) (struct super_block *);/*ɾ��������ķ���*/  
	//struct module *owner;/*ָ��ʵ���ļ�ϵͳ��ģ��ָ��*/  
	struct file_system_type * next;/*ָ���ļ�ϵͳ������������һ��Ԫ�ص�ָ��*/  
	struct list_head fs_supers;/*������ͬ�ļ�ϵͳ���͵ĳ������������ͷ*/  
  
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
