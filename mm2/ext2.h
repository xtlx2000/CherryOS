#ifndef _EXT2_H_
#define _EXT2_H_

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


/*
 * second extended file system inode data in memory
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



int init_ext2_fs();


#endif
