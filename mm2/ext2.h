#ifndef _EXT2_H_
#define _EXT2_H_

#include "list.h"
#include "bitmap.h"


//block
#define BLOCK_SIZE_BITS		12
#define BLOCK_SIZE	(1<<BLOCK_SIZE_BITS)
#define BITS_PER_BLOCK	(BLOCK_SIZE<<3)
#define UPPER(size,n)	((size +(n)-1)/(n))



//super block
struct ext2_super_block
{
	u32 s_inodes_count;			//inode数目//
	u32 s_blocks_count;			//block数目//
	u32 s_r_blocks_count;		//已分配的块数目//
	u32 s_free_blocks_count;	//空闲块数目//
	u32 s_free_inodes_count;	//空闲inode数目//
	u32 s_first_data_block;		//第一个数据块//
	u32 s_log_block_size;		//块长度//
	u32 s_log_frag_size;		//碎片长度//
	u32 s_blocks_per_group;		//每个快组包含的快数目//
	u32 s_frags_per_group;		//每个快组包含的碎片//
	u32 s_inodes_per_group;		//每个快组的inode数目//
	u32 s_state;				//zhuangtai

	u32 s_first_ino;			//第一个非保留的inode
	u32 s_inode_size;			//inode结构的大小

	u16 s_magic;

	virtual_device *dev;

	list_head  filesystem_type;
};


//gdt
#define GDT_SIZE (sizeof(ext2_group_desc))
struct ext2_group_desc
{
	u32 bg_block_bitmap;		//block位图块
	u32 bg_inode_bitmap;		//inode位图块
	u32 bg_inode_table;			//inode表块
	u16 bg_free_blocks_count;	//空闲块数目
	u16 bg_free_inodes_count;	//空闲inode数目
	u16 bg_used_dirs_count;		//目录数目
};


//inode in disk
#define	INODE_SIZE	(sizeof(ext2_inode))
#define INODES_PER_BLOCK	((BLOCK_SIZE)/(INODE_SIZE))
#define ROOT_INO (1)
#define EXT2_N_BLOCKS (4)//????

struct ext2_inode
{
	u16 i_mode;			//文件模式和权限
	u16 i_uid;			//所有者uid
	u32 i_size;			//长度
	u32 i_atime;		//访问时间
	u32 i_ctime;		//创建时间
	u32 i_mtime;		//修改时间
	u32 i_dtime;		//删除时间
	u16 i_gid;			//组ID的低16位
	u16 i_links_count;	//链接计数
	u32 i_blocks;		//块数目
	u32 i_flags;		//文件标志
	
	u32 i_block[EXT2_N_BLOCKS];	//块指针
	u32 i_file_acl;				//文件acl
	u32 i_dir_acl;				//目录acl
	u32 i_faddr;				//碎片地址
};

/*
 * ext2 inode in memory
 */
struct ext2_inode_info {
    u32  i_data[15];
    u32   i_flags;
    u32   i_faddr;
    u8    i_frag_no;
    u8    i_frag_size;
    u16   i_state;
    u32   i_file_acl;
    u32   i_dir_acl;
    u32   i_dtime;

    /*
     * i_block_group is the number of the block group which contains
     * this file's inode.  Constant across the lifetime of the inode,
     * it is ued for making block allocation decisions - we try to
     * place a file's data blocks near its inode block, and new inodes
     * near to their parent directory's inode.
     */
    u32   i_block_group;

    /* block reservation info */
    struct ext2_block_alloc_info *i_block_alloc_info;

    u32   i_dir_start_lookup;
#ifdef CONFIG_EXT2_FS_XATTR
    /*
     * Extended attributes can be read independently of the main file
     * data. Taking i_mutex even when reading would cause contention
     * between readers of EAs and writers of regular file data, so
     * instead we synchronize on xattr_sem when reading or changing
     * EAs.
     */
    struct rw_semaphore xattr_sem;
#endif
#ifdef CONFIG_EXT2_FS_POSIX_ACL
    struct posix_acl    *i_acl;
    struct posix_acl    *i_default_acl;
#endif
    //rwlock_t i_meta_lock;

    /*
     * truncate_mutex is for serialising ext2_truncate() against
     * ext2_getblock().  It also protects the internals of the inode's
     * reservation data structures: ext2_reserve_window and
     * ext2_reserve_window_node.
     */
    //struct mutex truncate_mutex;                             
	struct inode	vfs_inode;
	struct list_head i_orphan;	/* unlinked but open inodes */
};



#define NAME_LEN	60	
struct dir_entry {
	u32 ino;
	u32 name_len;
	char name[NAME_LEN];
};


#define EXT2_NAME_LEN 60

/*
 * The new version of the directory entry.  Since EXT2 structures are
 * stored in intel byte order, and the name_len field could never be
 * bigger than 255 chars, it's safe to reclaim the extra byte for the
 * file_type field.
 */
struct ext2_dir_entry_2 {
	u32	inode;			/* Inode number */
	u16	rec_len;		/* Directory entry length */
	u8	name_len;		/* Name length */
	u8	file_type;
	char	name[EXT2_NAME_LEN];	/* File name */
};





/* data type for block offset of block group */
typedef int ext2_grpblk_t;

/* data type for filesystem-wide blocks number */
typedef unsigned long ext2_fsblk_t;


struct ext2_reserve_window {                                                                                                                                                                                                     
    ext2_fsblk_t        _rsv_start; /* First byte reserved */
    ext2_fsblk_t        _rsv_end;   /* Last byte reserved or 0 */
};


struct ext2_reserve_window_node {                                                                                                                                                                                                
    struct rb_node      rsv_node;
    u32           rsv_goal_size;
    u32           rsv_alloc_hit; 
    struct ext2_reserve_window  rsv_window;
};


struct ext2_block_alloc_info {
    /* information about reservation window */
    struct ext2_reserve_window_node rsv_window_node;
    /*
     * was i_next_alloc_block in ext2_inode_info
     * is the logical (file-relative) number of the
     * most-recently-allocated block in this file.
     * We use this for detecting linearly ascending allocation requests.
     */
    u32           last_alloc_logical_block;
    /*
     * Was i_next_alloc_goal in ext2_inode_info
     * is the *physical* companion to i_next_alloc_block.
     * it the the physical block number of the block which was most-recentl
     * allocated to this file.  This give us the goal (target) for the next
     * allocation when we detect linearly ascending requests.
     */
    ext2_fsblk_t        last_alloc_physical_block;
};


#define EXT2_ROOT_INO 0




//-----------------网友ext2模拟用到的宏-----

#define EXT2_SUPER_MAGIC	0xEF53
/* from /usr/include/linux/magic.h */

#define EXT2_SUPER_MAGIC	0xEF53

/*
 * The second extended filesystem constants/structures
 */
 
/*
 * Define EXT2_RESERVATION to reserve data blocks for expanding files
 */
#define EXT2_DEFAULT_RESERVE_BLOCKS     8

/*max window size: 1024(direct blocks) + 3([t,d]indirect blocks) */
#define EXT2_MAX_RESERVE_BLOCKS         1027
#define EXT2_RESERVE_WINDOW_NOT_ALLOCATED 0

/*
 * The second extended file system version
 */
#define EXT2FS_DATE		"95/08/09"
#define EXT2FS_VERSION		"0.5b"

/*
 * Special inode numbers
 */
#define	EXT2_BAD_INO		 		1	/* Bad blocks inode */
#define EXT2_ROOT_INO		 		2	/* Root inode */
#define EXT2_BOOT_LOADER_INO	 	5	/* Boot loader inode */
#define EXT2_UNDEL_DIR_INO	 		6	/* Undelete directory inode */

/* First non-reserved inode for old ext2 filesystems */
#define EXT2_GOOD_OLD_FIRST_INO	11


/* Assume that user mode programs are passing in an ext2fs superblock, not
 * a kernel struct super_block.  This will allow us to call the feature-test
 * macros from user land. */
#define EXT2_SB(sb)	(sb)

/*
 * Maximal count of links to a file
 */
#define EXT2_LINK_MAX		32000

/*
 * Macro-instructions used to manage several block sizes
 */
#define EXT2_MIN_BLOCK_SIZE		1024
#define	EXT2_MAX_BLOCK_SIZE		4096
#define EXT2_MIN_BLOCK_LOG_SIZE		  10
#define EXT2_BLOCK_SIZE(s)		(EXT2_MIN_BLOCK_SIZE << (s)->s_log_block_size)
#define	EXT2_ADDR_PER_BLOCK(s)		(EXT2_BLOCK_SIZE(s) / sizeof (uint32_t))
#define EXT2_BLOCK_SIZE_BITS(s)	((s)->s_log_block_size + 10)
#define EXT2_INODE_SIZE(s)	(((s)->s_rev_level == EXT2_GOOD_OLD_REV) ? \
				 EXT2_GOOD_OLD_INODE_SIZE : \
				 (s)->s_inode_size)
#define EXT2_FIRST_INO(s)	(((s)->s_rev_level == EXT2_GOOD_OLD_REV) ? \
				 EXT2_GOOD_OLD_FIRST_INO : \
				 (s)->s_first_ino)


/*
 * Macro-instructions used to manage fragments
 */
#define EXT2_MIN_FRAG_SIZE		1024
#define	EXT2_MAX_FRAG_SIZE		4096
#define EXT2_MIN_FRAG_LOG_SIZE		  10
#define EXT2_FRAG_SIZE(s)		(EXT2_MIN_FRAG_SIZE << (s)->s_log_frag_size)
#define EXT2_FRAGS_PER_BLOCK(s)	(EXT2_BLOCK_SIZE(s) / EXT2_FRAG_SIZE(s))


/*
 * Macro-instructions used to manage group descriptors
 */
#define EXT2_BLOCKS_PER_GROUP(s)	((s)->s_blocks_per_group)
#define EXT2_DESC_PER_BLOCK(s)		(EXT2_BLOCK_SIZE(s) / sizeof (struct ext2_group_desc))
#define EXT2_INODES_PER_GROUP(s)	((s)->s_inodes_per_group)


/*
 * Constants relative to the data blocks
 */
#define	EXT2_NDIR_BLOCKS		12
#define	EXT2_IND_BLOCK			EXT2_NDIR_BLOCKS
#define	EXT2_DIND_BLOCK			(EXT2_IND_BLOCK + 1)
#define	EXT2_TIND_BLOCK			(EXT2_DIND_BLOCK + 1)
#define	EXT2_N_BLOCKS			(EXT2_TIND_BLOCK + 1)

/*
 * Inode flags (GETFLAGS/SETFLAGS)
 */
#define	EXT2_SECRM_FL			FS_SECRM_FL	/* Secure deletion */
#define	EXT2_UNRM_FL			FS_UNRM_FL	/* Undelete */
#define	EXT2_COMPR_FL			FS_COMPR_FL	/* Compress file */
#define EXT2_SYNC_FL			FS_SYNC_FL	/* Synchronous updates */
#define EXT2_IMMUTABLE_FL		FS_IMMUTABLE_FL	/* Immutable file */
#define EXT2_APPEND_FL			FS_APPEND_FL	/* writes to file may only append */
#define EXT2_NODUMP_FL			FS_NODUMP_FL	/* do not dump file */
#define EXT2_NOATIME_FL			FS_NOATIME_FL	/* do not update atime */
/* Reserved for compression usage... */
#define EXT2_DIRTY_FL			FS_DIRTY_FL
#define EXT2_COMPRBLK_FL		FS_COMPRBLK_FL	/* One or more compressed clusters */
#define EXT2_NOCOMP_FL			FS_NOCOMP_FL	/* Don't compress */
#define EXT2_ECOMPR_FL			FS_ECOMPR_FL	/* Compression error */
/* End compression flags --- maybe not all used */
#define EXT2_BTREE_FL			FS_BTREE_FL	/* btree format dir */
#define EXT2_INDEX_FL			FS_INDEX_FL	/* hash-indexed directory */
#define EXT2_IMAGIC_FL			FS_IMAGIC_FL	/* AFS directory */
#define EXT2_JOURNAL_DATA_FL		FS_JOURNAL_DATA_FL /* Reserved for ext3 */
#define EXT2_NOTAIL_FL			FS_NOTAIL_FL	/* file tail should not be merged */
#define EXT2_DIRSYNC_FL			FS_DIRSYNC_FL	/* dirsync behaviour (directories only) */
#define EXT2_TOPDIR_FL			FS_TOPDIR_FL	/* Top of directory hierarchies*/
#define EXT2_RESERVED_FL		FS_RESERVED_FL	/* reserved for ext2 lib */

#define EXT2_FL_USER_VISIBLE		FS_FL_USER_VISIBLE	/* User visible flags */
#define EXT2_FL_USER_MODIFIABLE		FS_FL_USER_MODIFIABLE	/* User modifiable flags */


/*
 * ioctl commands
 */
#define	EXT2_IOC_GETFLAGS		FS_IOC_GETFLAGS
#define	EXT2_IOC_SETFLAGS		FS_IOC_SETFLAGS
#define	EXT2_IOC_GETVERSION		FS_IOC_GETVERSION
#define	EXT2_IOC_SETVERSION		FS_IOC_SETVERSION
#define	EXT2_IOC_GETRSVSZ		_IOR('f', 5, long)
#define	EXT2_IOC_SETRSVSZ		_IOW('f', 6, long)

/*
 * ioctl commands in 32 bit emulation
 */
#define EXT2_IOC32_GETFLAGS		FS_IOC32_GETFLAGS
#define EXT2_IOC32_SETFLAGS		FS_IOC32_SETFLAGS
#define EXT2_IOC32_GETVERSION		FS_IOC32_GETVERSION
#define EXT2_IOC32_SETVERSION		FS_IOC32_SETVERSION


/*
 * File system states
 */
#define	EXT2_VALID_FS			0x0001	/* Unmounted cleanly */
#define	EXT2_ERROR_FS			0x0002	/* Errors detected */

/*
 * Mount flags
 */
#define EXT2_MOUNT_CHECK		0x000001  /* Do mount-time checks */
#define EXT2_MOUNT_OLDALLOC		0x000002  /* Don't use the new Orlov allocator */
#define EXT2_MOUNT_GRPID		0x000004  /* Create files with directory's group */
#define EXT2_MOUNT_DEBUG		0x000008  /* Some debugging messages */
#define EXT2_MOUNT_ERRORS_CONT		0x000010  /* Continue on errors */
#define EXT2_MOUNT_ERRORS_RO		0x000020  /* Remount fs ro on errors */
#define EXT2_MOUNT_ERRORS_PANIC		0x000040  /* Panic on errors */
#define EXT2_MOUNT_MINIX_DF		0x000080  /* Mimics the Minix statfs */
#define EXT2_MOUNT_NOBH			0x000100  /* No buffer_heads */
#define EXT2_MOUNT_NO_UID32		0x000200  /* Disable 32-bit UIDs */
#define EXT2_MOUNT_XATTR_USER		0x004000  /* Extended user attributes */
#define EXT2_MOUNT_POSIX_ACL		0x008000  /* POSIX Access Control Lists */
#define EXT2_MOUNT_XIP			0x010000  /* Execute in place */
#define EXT2_MOUNT_USRQUOTA		0x020000  /* user quota */
#define EXT2_MOUNT_GRPQUOTA		0x040000  /* group quota */
#define EXT2_MOUNT_RESERVATION		0x080000  /* Preallocation */


#define clear_opt(o, opt)		o &= ~EXT2_MOUNT_##opt
#define set_opt(o, opt)			o |= EXT2_MOUNT_##opt
#define test_opt(sb, opt)		(EXT2_SB(sb)->s_mount_opt & \
					 EXT2_MOUNT_##opt)

/*
 * Maximal mount counts between two filesystem checks
 */
#define EXT2_DFL_MAX_MNT_COUNT		20	/* Allow 20 mounts */
#define EXT2_DFL_CHECKINTERVAL		0	/* Don't use interval check */

/*
 * Behaviour when detecting errors
 */
#define EXT2_ERRORS_CONTINUE		1	/* Continue execution */
#define EXT2_ERRORS_RO			2	/* Remount fs read-only */
#define EXT2_ERRORS_PANIC		3	/* Panic */
#define EXT2_ERRORS_DEFAULT		EXT2_ERRORS_CONTINUE


/*
 * Codes for operating systems
 */
#define EXT2_OS_LINUX		0
#define EXT2_OS_HURD		1
#define EXT2_OS_MASIX		2
#define EXT2_OS_FREEBSD		3
#define EXT2_OS_LITES		4

/*
 * Revision levels
 */
#define EXT2_GOOD_OLD_REV	0	/* The good old (original) format */
#define EXT2_DYNAMIC_REV	1 	/* V2 format w/ dynamic inode sizes */

#define EXT2_CURRENT_REV	EXT2_GOOD_OLD_REV
#define EXT2_MAX_SUPP_REV	EXT2_DYNAMIC_REV

#define EXT2_GOOD_OLD_INODE_SIZE 128


/*
 * Feature set definitions
 */

#define EXT2_HAS_COMPAT_FEATURE(sb,mask)			\
	( EXT2_SB(sb)->s_es->s_feature_compat & cpu_to_le32(mask) )
#define EXT2_HAS_RO_COMPAT_FEATURE(sb,mask)			\
	( EXT2_SB(sb)->s_es->s_feature_ro_compat & cpu_to_le32(mask) )
#define EXT2_HAS_INCOMPAT_FEATURE(sb,mask)			\
	( EXT2_SB(sb)->s_es->s_feature_incompat & cpu_to_le32(mask) )
#define EXT2_SET_COMPAT_FEATURE(sb,mask)			\
	EXT2_SB(sb)->s_es->s_feature_compat |= cpu_to_le32(mask)
#define EXT2_SET_RO_COMPAT_FEATURE(sb,mask)			\
	EXT2_SB(sb)->s_es->s_feature_ro_compat |= cpu_to_le32(mask)
#define EXT2_SET_INCOMPAT_FEATURE(sb,mask)			\
	EXT2_SB(sb)->s_es->s_feature_incompat |= cpu_to_le32(mask)
#define EXT2_CLEAR_COMPAT_FEATURE(sb,mask)			\
	EXT2_SB(sb)->s_es->s_feature_compat &= ~cpu_to_le32(mask)
#define EXT2_CLEAR_RO_COMPAT_FEATURE(sb,mask)			\
	EXT2_SB(sb)->s_es->s_feature_ro_compat &= ~cpu_to_le32(mask)
#define EXT2_CLEAR_INCOMPAT_FEATURE(sb,mask)			\
	EXT2_SB(sb)->s_es->s_feature_incompat &= ~cpu_to_le32(mask)


#define EXT2_FEATURE_COMPAT_DIR_PREALLOC	0x0001
#define EXT2_FEATURE_COMPAT_IMAGIC_INODES	0x0002
#define EXT3_FEATURE_COMPAT_HAS_JOURNAL		0x0004
#define EXT2_FEATURE_COMPAT_EXT_ATTR		0x0008
#define EXT2_FEATURE_COMPAT_RESIZE_INO		0x0010
#define EXT2_FEATURE_COMPAT_DIR_INDEX		0x0020
#define EXT2_FEATURE_COMPAT_ANY			0xffffffff

#define EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER	0x0001
#define EXT2_FEATURE_RO_COMPAT_LARGE_FILE	0x0002
#define EXT2_FEATURE_RO_COMPAT_BTREE_DIR	0x0004
#define EXT2_FEATURE_RO_COMPAT_ANY		0xffffffff

#define EXT2_FEATURE_INCOMPAT_COMPRESSION	0x0001
#define EXT2_FEATURE_INCOMPAT_FILETYPE		0x0002
#define EXT3_FEATURE_INCOMPAT_RECOVER		0x0004
#define EXT3_FEATURE_INCOMPAT_JOURNAL_DEV	0x0008
#define EXT2_FEATURE_INCOMPAT_META_BG		0x0010
#define EXT2_FEATURE_INCOMPAT_ANY		0xffffffff

#define EXT2_FEATURE_COMPAT_SUPP	EXT2_FEATURE_COMPAT_EXT_ATTR
#define EXT2_FEATURE_INCOMPAT_SUPP	(EXT2_FEATURE_INCOMPAT_FILETYPE| \
					 EXT2_FEATURE_INCOMPAT_META_BG)
#define EXT2_FEATURE_RO_COMPAT_SUPP	(EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER| \
					 EXT2_FEATURE_RO_COMPAT_LARGE_FILE| \
					 EXT2_FEATURE_RO_COMPAT_BTREE_DIR)
#define EXT2_FEATURE_RO_COMPAT_UNSUPPORTED	~EXT2_FEATURE_RO_COMPAT_SUPP
#define EXT2_FEATURE_INCOMPAT_UNSUPPORTED	~EXT2_FEATURE_INCOMPAT_SUPP

/*
 * Default values for user and/or group using reserved blocks
 */
#define	EXT2_DEF_RESUID		0
#define	EXT2_DEF_RESGID		0

/*
 * Default mount options
 */
#define EXT2_DEFM_DEBUG		0x0001
#define EXT2_DEFM_BSDGROUPS	0x0002
#define EXT2_DEFM_XATTR_USER	0x0004
#define EXT2_DEFM_ACL		0x0008
#define EXT2_DEFM_UID16		0x0010
/* Not used by ext2, but reserved for use by ext3 */
#define EXT3_DEFM_JMODE		0x0060
#define EXT3_DEFM_JMODE_DATA	0x0020
#define EXT3_DEFM_JMODE_ORDERED	0x0040
#define EXT3_DEFM_JMODE_WBACK	0x0060


/*
 * EXT2_DIR_PAD defines the directory entries boundaries
 *
 * NOTE: It must be a multiple of 4
 */
#define EXT2_DIR_PAD		 	4
#define EXT2_DIR_ROUND 			(EXT2_DIR_PAD - 1)
#define EXT2_DIR_REC_LEN(name_len)	(((name_len) + 8 + EXT2_DIR_ROUND) & \
					 ~EXT2_DIR_ROUND)
#define EXT2_MAX_REC_LEN		((1<<16)-1)	



typedef struct ext2_DIR {
        ext2_VOLUME *volume;
	struct ext2_inode *inode;
	off_t index;
} ext2_DIR;


typedef struct ext2_VOLUME {
        int fd;
	struct ext2_super_block *super;
	unsigned int current;
	char *buffer;
} ext2_VOLUME;

typedef struct ext2_FILE {
        ext2_VOLUME *volume;
	struct ext2_inode *inode;
	off_t offset;
	char *path;
} ext2_FILE;


//---------------------------------------------------



int init_ext2_fs();
int format_to_ext2(struct virtual_device *dev);
int sys_mount();
int sys_umount();





#endif
