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







int init_ext2_fs();
int format_to_ext2(struct virtual_device *dev);
int sys_mount();
int sys_umount();





#endif
