#include "ext2.h"
#include "errno.h"
#include "string.h"


static struct kmem_cache * ext2_inode_cachep;



static int ext2_get_sb(struct file_system_type *fs_type,                                                    
    int flags, const char *dev_name, void *data, struct vfsmount *mnt);
struct super_block *sget(struct file_system_type *type,
            int (*test)(struct super_block *,void *),
            int (*set)(struct super_block *,void *),
            void *data);
static int grab_super(struct super_block *s);







static struct file_system_type ext2_fs_type = {                                                             
    .name       = "ext2",
    .get_sb     = ext2_get_sb,
    .kill_sb    = kill_block_super,
    .fs_flags   = 0,
    .next 		= NULL
};


//注册ext2文件系统到OS
int init_ext2_fs(void)
{
	int err;
	/*
    err = init_ext2_xattr();
    if (err)
        return err;
       */
    err = init_inodecache();
    if (err)
        goto out1;

    err = register_filesystem(&ext2_fs_type);//注册
    if (err)
        goto out;
    return 0;
out:
    destroy_inodecache();
out1:
    //exit_ext2_xattr();
    return err;
}




int init_inodecache(void)                                                                            
{
    ext2_inode_cachep = kmem_cache_create("ext2_inode_cache",
                         sizeof(struct ext2_inode_info),
                         0, 
                         NULL,
                         NULL);
    if (ext2_inode_cachep == NULL)
        return -ENOMEM;
    return 0;
}



void destroy_inodecache(void)
{
    kmem_cache_destroy(ext2_inode_cachep);
}


static int ext2_get_sb(struct file_system_type *fs_type,                                                    
    int flags, const char *dev_name, void *data, struct vfsmount *mnt)
{   
    return get_sb_bdev(fs_type, flags, dev_name, data, ext2_fill_super, mnt);
} 


/*
  * get_sb_bdev中调用sget新建一个super_block，之后调用ext2_fill_super，
  * ext2_fill_super中调用ext2_iget新建一个inode，该inode作为挂载点的虚拟根，
  * 之后调用d_alloc_root新建一个dentry
  */
int get_sb_bdev(struct file_system_type *fs_type,
    int flags, const char *dev_name, void *data,
    int (*fill_super)(struct super_block *, void *, int),
    struct vfsmount *mnt)
{
    struct block_device *bdev;
    struct super_block *s;
    int error = 0;

    //bdev = open_bdev_excl(dev_name, flags, fs_type);
    //if (IS_ERR(bdev))
    //    return PTR_ERR(bdev);

    /*
     * once the super is inserted into the list by sget, s_umount
     * will protect the lockfs code from trying to start a snapshot
     * while we are mounting
     */
    //down(&bdev->bd_mount_sem);
    s = sget(fs_type, test_bdev_super, set_bdev_super, bdev);
    up(&bdev->bd_mount_sem);
    if (IS_ERR(s))
        goto error_s;

    if (s->s_root) {
        if ((flags ^ s->s_flags) & MS_RDONLY) {
            up_write(&s->s_umount);
            deactivate_super(s);
            error = -EBUSY;
            goto error_bdev;
        }

        close_bdev_excl(bdev);
    } else {
        char b[BDEVNAME_SIZE];

        s->s_flags = flags;
        strlcpy(s->s_id, bdevname(bdev, b), sizeof(s->s_id));
        sb_set_blocksize(s, block_size(bdev));
        error = fill_super(s, data, flags & MS_SILENT ? 1 : 0);
        if (error) {
            up_write(&s->s_umount);
            deactivate_super(s);
            goto error;
        }

        s->s_flags |= MS_ACTIVE;
    }

    return simple_set_mnt(mnt, s);
	
error_s:
	error = PTR_ERR(s);
error_bdev:
	close_bdev_excl(bdev);
error:
	return error;
}


static int ext2_fill_super(struct super_block *sb, void *data, int silent)
{
    struct buffer_head * bh;
    struct ext2_sb_info * sbi;
    struct ext2_super_block * es;
    struct inode *root;
    unsigned long block;
    unsigned long sb_block = get_sb_block(&data);
    unsigned long logic_sb_block;
    unsigned long offset = 0;
    unsigned long def_mount_opts;
    long ret = -EINVAL;
    int blocksize = BLOCK_SIZE;
    int db_count;
    int i, j;
    __le32 features;
    int err;

    sbi = kzalloc(sizeof(*sbi), GFP_KERNEL);
    if (!sbi)
        return -ENOMEM;
    sb->s_fs_info = sbi;
    sbi->s_sb_block = sb_block;

    /*
     * See what the current blocksize for the device is, and
     * use that as the blocksize.  Otherwise (or if the blocksize
     * is smaller than the default) use the default.
     * This is important for devices that have a hardware
     * sectorsize that is larger than the default.
     */
    blocksize = sb_min_blocksize(sb, BLOCK_SIZE);
    if (!blocksize) {
        printk ("EXT2-fs: unable to set blocksize\n");
        goto failed_sbi;
    }

    /*
     * If the superblock doesn't start on a hardware sector boundary,
     * calculate the offset.  
     */
    if (blocksize != BLOCK_SIZE) {
        logic_sb_block = (sb_block*BLOCK_SIZE) / blocksize;
        offset = (sb_block*BLOCK_SIZE) % blocksize;
    } else {
        logic_sb_block = sb_block;
    }

    if (!(bh = sb_bread(sb, logic_sb_block))) {            
        printk ("EXT2-fs: unable to read superblock\n");
        goto failed_sbi;
    }
    /*
     * Note: s_es must be initialized as soon as possible because
     *       some ext2 macro-instructions depend on its value
     */
    es = (struct ext2_super_block *) (((char *)bh->b_data) + offset);
    sbi->s_es = es;
    sb->s_magic = le16_to_cpu(es->s_magic);

    if (sb->s_magic != EXT2_SUPER_MAGIC)
        goto cantfind_ext2;

    /* Set defaults before we parse the mount options */
    def_mount_opts = le32_to_cpu(es->s_default_mount_opts);
    if (def_mount_opts & EXT2_DEFM_DEBUG)
        set_opt(sbi->s_mount_opt, DEBUG);
    if (def_mount_opts & EXT2_DEFM_BSDGROUPS)
        set_opt(sbi->s_mount_opt, GRPID);
    if (def_mount_opts & EXT2_DEFM_UID16)
        set_opt(sbi->s_mount_opt, NO_UID32);
#ifdef CONFIG_EXT2_FS_XATTR
    if (def_mount_opts & EXT2_DEFM_XATTR_USER)
        set_opt(sbi->s_mount_opt, XATTR_USER);
#endif
#ifdef CONFIG_EXT2_FS_POSIX_ACL
    if (def_mount_opts & EXT2_DEFM_ACL)
        set_opt(sbi->s_mount_opt, POSIX_ACL);
#endif

    if (le16_to_cpu(sbi->s_es->s_errors) == EXT2_ERRORS_PANIC)
        set_opt(sbi->s_mount_opt, ERRORS_PANIC);
    else if (le16_to_cpu(sbi->s_es->s_errors) == EXT2_ERRORS_CONTINUE)
        set_opt(sbi->s_mount_opt, ERRORS_CONT);
    else
        set_opt(sbi->s_mount_opt, ERRORS_RO);

    sbi->s_resuid = le16_to_cpu(es->s_def_resuid);
    sbi->s_resgid = le16_to_cpu(es->s_def_resgid);

    set_opt(sbi->s_mount_opt, RESERVATION);

    if (!parse_options ((char *) data, sbi))
        goto failed_mount;

    sb->s_flags = (sb->s_flags & ~MS_POSIXACL) |
            ((EXT2_SB(sb)->s_mount_opt & EXT2_MOUNT_POSIX_ACL) ?
         	MS_POSIXACL : 0);

    ext2_xip_verify_sb(sb); /* see if bdev supports xip, unset
                    EXT2_MOUNT_XIP if not */

    if (le32_to_cpu(es->s_rev_level) == EXT2_GOOD_OLD_REV &&
        (EXT2_HAS_COMPAT_FEATURE(sb, ~0U) ||
         EXT2_HAS_RO_COMPAT_FEATURE(sb, ~0U) ||
         EXT2_HAS_INCOMPAT_FEATURE(sb, ~0U)))
        printk("EXT2-fs warning: feature flags set on rev 0 fs, "
               "running e2fsck is recommended\n");
    /*
     * Check feature flags regardless of the revision level, since we
     * previously didn't change the revision level when setting the flags,
     * so there is a chance incompat flags are set on a rev 0 filesystem.
     */
    features = EXT2_HAS_INCOMPAT_FEATURE(sb, ~EXT2_FEATURE_INCOMPAT_SUPP);
    if (features) {
        printk("EXT2-fs: %s: couldn't mount because of "
               "unsupported optional features (%x).\n",
               sb->s_id, le32_to_cpu(features));
        goto failed_mount;
    }
    if (!(sb->s_flags & MS_RDONLY) &&
        (features = EXT2_HAS_RO_COMPAT_FEATURE(sb, ~EXT2_FEATURE_RO_COMPAT_SUPP))){
        printk("EXT2-fs: %s: couldn't mount RDWR because of "
               "unsupported optional features (%x).\n",
               sb->s_id, le32_to_cpu(features));
        goto failed_mount;
    }

    blocksize = BLOCK_SIZE << le32_to_cpu(sbi->s_es->s_log_block_size);

    if (ext2_use_xip(sb) && blocksize != PAGE_SIZE) {
        if (!silent)
            printk("XIP: Unsupported blocksize\n");
        goto failed_mount;
    }

    /* If the blocksize doesn't match, re-read the thing.. */
    if (sb->s_blocksize != blocksize) {
        brelse(bh);

        if (!sb_set_blocksize(sb, blocksize)) {
            printk(KERN_ERR "EXT2-fs: blocksize too small for device.\n");
            goto failed_sbi;
        }

        logic_sb_block = (sb_block*BLOCK_SIZE) / blocksize;
        offset = (sb_block*BLOCK_SIZE) % blocksize;
        bh = sb_bread(sb, logic_sb_block);
        if(!bh) {
            printk("EXT2-fs: Couldn't read superblock on "
                   "2nd try.\n");
            goto failed_sbi;
        }
        es = (struct ext2_super_block *) (((char *)bh->b_data) + offset);
        sbi->s_es = es;
        if (es->s_magic != cpu_to_le16(EXT2_SUPER_MAGIC)) {
            printk ("EXT2-fs: Magic mismatch, very weird !\n");
            goto failed_mount;
        }
    }

    sb->s_maxbytes = ext2_max_size(sb->s_blocksize_bits);

    if (le32_to_cpu(es->s_rev_level) == EXT2_GOOD_OLD_REV) {
        sbi->s_inode_size = EXT2_GOOD_OLD_INODE_SIZE;
        sbi->s_first_ino = EXT2_GOOD_OLD_FIRST_INO;
    } else {
        sbi->s_inode_size = le16_to_cpu(es->s_inode_size);
        sbi->s_first_ino = le32_to_cpu(es->s_first_ino);
        if ((sbi->s_inode_size < EXT2_GOOD_OLD_INODE_SIZE) ||
            !is_power_of_2(sbi->s_inode_size) ||
            (sbi->s_inode_size > blocksize)) {
            printk ("EXT2-fs: unsupported inode size: %d\n",
                sbi->s_inode_size);
            goto failed_mount;
        }
    }

    sbi->s_frag_size = EXT2_MIN_FRAG_SIZE <<
                   le32_to_cpu(es->s_log_frag_size);
    if (sbi->s_frag_size == 0)
        goto cantfind_ext2;
    sbi->s_frags_per_block = sb->s_blocksize / sbi->s_frag_size;

    sbi->s_blocks_per_group = le32_to_cpu(es->s_blocks_per_group);
    sbi->s_frags_per_group = le32_to_cpu(es->s_frags_per_group);
    sbi->s_inodes_per_group = le32_to_cpu(es->s_inodes_per_group);

    if (EXT2_INODE_SIZE(sb) == 0)
        goto cantfind_ext2;
    sbi->s_inodes_per_block = sb->s_blocksize / EXT2_INODE_SIZE(sb);
    if (sbi->s_inodes_per_block == 0 || sbi->s_inodes_per_group == 0)
        goto cantfind_ext2;
    sbi->s_itb_per_group = sbi->s_inodes_per_group /
                    sbi->s_inodes_per_block;
    sbi->s_desc_per_block = sb->s_blocksize /
                    sizeof (struct ext2_group_desc);
    sbi->s_sbh = bh;
    sbi->s_mount_state = le16_to_cpu(es->s_state);
    sbi->s_addr_per_block_bits =
        ilog2 (EXT2_ADDR_PER_BLOCK(sb));
    sbi->s_desc_per_block_bits =
        ilog2 (EXT2_DESC_PER_BLOCK(sb));

    if (sb->s_magic != EXT2_SUPER_MAGIC)
        goto cantfind_ext2;

    if (sb->s_blocksize != bh->b_size) {
        if (!silent)
            printk ("VFS: Unsupported blocksize on dev "
                "%s.\n", sb->s_id);
        goto failed_mount;
    }

    if (sb->s_blocksize != sbi->s_frag_size) {
        printk ("EXT2-fs: fragsize %lu != blocksize %lu (not supported yet)\n",
            sbi->s_frag_size, sb->s_blocksize);
        goto failed_mount;
    }

    if (sbi->s_blocks_per_group > sb->s_blocksize * 8) {
        printk ("EXT2-fs: #blocks per group too big: %lu\n",
            sbi->s_blocks_per_group);
        goto failed_mount;
    }
    if (sbi->s_frags_per_group > sb->s_blocksize * 8) {
        printk ("EXT2-fs: #fragments per group too big: %lu\n",
            sbi->s_frags_per_group);
        goto failed_mount;
    }
    if (sbi->s_inodes_per_group > sb->s_blocksize * 8) {
        printk ("EXT2-fs: #inodes per group too big: %lu\n",
            sbi->s_inodes_per_group);
        goto failed_mount;
    }

    if (EXT2_BLOCKS_PER_GROUP(sb) == 0)
        goto cantfind_ext2;
    sbi->s_groups_count = ((le32_to_cpu(es->s_blocks_count) -
                le32_to_cpu(es->s_first_data_block) - 1)
                    / EXT2_BLOCKS_PER_GROUP(sb)) + 1;
    db_count = (sbi->s_groups_count + EXT2_DESC_PER_BLOCK(sb) - 1) /
           EXT2_DESC_PER_BLOCK(sb);
    sbi->s_group_desc = kmalloc (db_count * sizeof (struct buffer_head *), GFP_KERNEL);
    if (sbi->s_group_desc == NULL) {
        printk ("EXT2-fs: not enough memory\n");
        goto failed_mount;
    }
    bgl_lock_init(&sbi->s_blockgroup_lock);
    sbi->s_debts = kcalloc(sbi->s_groups_count, sizeof(*sbi->s_debts), GFP_KERNEL);
    if (!sbi->s_debts) {
        printk ("EXT2-fs: not enough memory\n");
        goto failed_mount_group_desc;
    }
    for (i = 0; i < db_count; i++) {
        block = descriptor_loc(sb, logic_sb_block, i);
        sbi->s_group_desc[i] = sb_bread(sb, block);
        if (!sbi->s_group_desc[i]) {
            for (j = 0; j < i; j++)
                brelse (sbi->s_group_desc[j]);
            printk ("EXT2-fs: unable to read group descriptors\n");
            goto failed_mount_group_desc;
        }
    }
    if (!ext2_check_descriptors (sb)) {
        printk ("EXT2-fs: group descriptors corrupted!\n");
        goto failed_mount2;
    }
    sbi->s_gdb_count = db_count;
    get_random_bytes(&sbi->s_next_generation, sizeof(u32));
    spin_lock_init(&sbi->s_next_gen_lock);

    /* per fileystem reservation list head & lock */
    spin_lock_init(&sbi->s_rsv_window_lock);
    sbi->s_rsv_window_root = RB_ROOT;
    /*
     * Add a single, static dummy reservation to the start of the
     * reservation window list --- it gives us a placeholder for
     * append-at-start-of-list which makes the allocation logic
     * _much_ simpler.
     */
    sbi->s_rsv_window_head.rsv_start = EXT2_RESERVE_WINDOW_NOT_ALLOCATED;
    sbi->s_rsv_window_head.rsv_end = EXT2_RESERVE_WINDOW_NOT_ALLOCATED;
    sbi->s_rsv_window_head.rsv_alloc_hit = 0;
    sbi->s_rsv_window_head.rsv_goal_size = 0;
    ext2_rsv_window_add(sb, &sbi->s_rsv_window_head);

    err = percpu_counter_init(&sbi->s_freeblocks_counter,
                ext2_count_free_blocks(sb));
    if (!err) {
        err = percpu_counter_init(&sbi->s_freeinodes_counter,
                ext2_count_free_inodes(sb));
    }
    if (!err) {
        err = percpu_counter_init(&sbi->s_dirs_counter,
                ext2_count_dirs(sb));
    }
    if (err) {
        printk(KERN_ERR "EXT2-fs: insufficient memory\n");
        goto failed_mount3;
    }
    /*
     * set up enough so that it can read an inode
     */
    sb->s_op = &ext2_sops;
    sb->s_export_op = &ext2_export_ops;
    sb->s_xattr = ext2_xattr_handlers;
    root = ext2_iget(sb, EXT2_ROOT_INO);
    if (IS_ERR(root)) {
        ret = PTR_ERR(root);
        goto failed_mount3;
    }
    if (!S_ISDIR(root->i_mode) || !root->i_blocks || !root->i_size) {
        iput(root);
        printk(KERN_ERR "EXT2-fs: corrupt root inode, run e2fsck\n");
        goto failed_mount3;
    }

    sb->s_root = d_alloc_root(root);
    if (!sb->s_root) {
        iput(root);
        printk(KERN_ERR "EXT2-fs: get root inode failed\n");
        ret = -ENOMEM;
        goto failed_mount3;
    }
    if (EXT2_HAS_COMPAT_FEATURE(sb, EXT3_FEATURE_COMPAT_HAS_JOURNAL))
        ext2_warning(sb, __func__,
            "mounting ext3 filesystem as ext2");
    ext2_setup_super (sb, es, sb->s_flags & MS_RDONLY);
    return 0;
	
cantfind_ext2:
	if (!silent)
		printk("VFS: Can't find an ext2 filesystem on dev %s.\n",
			   sb->s_id);
	goto failed_mount;
failed_mount3:
	percpu_counter_destroy(&sbi->s_freeblocks_counter);
	percpu_counter_destroy(&sbi->s_freeinodes_counter);
	percpu_counter_destroy(&sbi->s_dirs_counter);
failed_mount2:
	for (i = 0; i < db_count; i++)
		brelse(sbi->s_group_desc[i]);
failed_mount_group_desc:
	kfree(sbi->s_group_desc);
	kfree(sbi->s_debts);
failed_mount:
	brelse(bh);
failed_sbi:
	sb->s_fs_info = NULL;
	kfree(sbi);
	return ret;
}




/**                                                                                                                                                                                                                              
 *  sget    -   find or create a superblock
 *  @type:  filesystem type superblock should belong to
 *  @test:  comparison callback
 *  @set:   setup callback
 *  @data:  argument to each of them
 */
struct super_block *sget(struct file_system_type *type,
            int (*test)(struct super_block *,void *),
            int (*set)(struct super_block *,void *),
            void *data)
{
    struct super_block *s = NULL;
    struct super_block *old;
    int err;

retry:
    //spin_lock(&sb_lock);
    if (test) {
        list_for_each_entry(old, &type->fs_supers, s_instances) {
            if (!test(old, data))
                continue;
            if (!grab_super(old))
                goto retry;
            if (s)
                destroy_super(s);
            return old;
        }
    }
    if (!s) {
        //spin_unlock(&sb_lock);
        s = alloc_super(type);
        if (!s)
            return ERR_PTR(-ENOMEM);
        goto retry;
    }
        
    err = set(s, data);
    if (err) {
        spin_unlock(&sb_lock);
        destroy_super(s);
        return ERR_PTR(err);
    }
    s->s_type = type;
    strlcpy(s->s_id, type->name, sizeof(s->s_id));
    list_add_tail(&s->s_list, &super_blocks);
    list_add(&s->s_instances, &type->fs_supers);
    spin_unlock(&sb_lock);
    get_filesystem(type);
    return s;
}


/** 
 *  grab_super - acquire an active reference
 *  @s: reference we are trying to make active
 *
 *  Tries to acquire an active reference.  grab_super() is used when we
 *  had just found a superblock in super_blocks or fs_type->fs_supers
 *  and want to turn it into a full-blown active reference.  grab_super()
 *  is called with sb_lock held and drops it.  Returns 1 in case of
 *  success, 0 if we had failed (superblock contents was already dead or
 *  dying when grab_super() had been called).
 */
static int grab_super(struct super_block *s)
{
    s->s_count++;
    spin_unlock(&sb_lock);
    down_write(&s->s_umount);
    if (s->s_root) {
        spin_lock(&sb_lock);
        if (s->s_count > S_BIAS) {
            atomic_inc(&s->s_active);
            s->s_count--;
            spin_unlock(&sb_lock);
            return 1;
        }
        spin_unlock(&sb_lock);
    }
    up_write(&s->s_umount);
    put_super(s);
    yield();
    return 0;
}


/**
 *  destroy_super   -   frees a superblock
 *  @s: superblock to free
 *      
 *  Frees a superblock.
 */             
static inline void destroy_super(struct super_block *s)                                                     
{               
    security_sb_free(s);
    kfree(s->s_subtype);
    kfree(s->s_options);
    kfree(s);
} 


/**
 *  alloc_super -   create new superblock
 *  @type:  filesystem type superblock should belong to
 *
 *  Allocates and initializes a new &struct super_block.  alloc_super()
 *  returns a pointer new superblock or %NULL if allocation had failed.
 */
static struct super_block *alloc_super(struct file_system_type *type)
{
    struct super_block *s = kzalloc(sizeof(struct super_block),  GFP_USER);
    static struct super_operations default_op;

    if (s) {
        if (security_sb_alloc(s)) {
            kfree(s);
            s = NULL;
            goto out;
        }
        INIT_LIST_HEAD(&s->s_dirty);
        INIT_LIST_HEAD(&s->s_io);
        INIT_LIST_HEAD(&s->s_more_io);
        INIT_LIST_HEAD(&s->s_files);
        INIT_LIST_HEAD(&s->s_instances);
        INIT_HLIST_HEAD(&s->s_anon);
        INIT_LIST_HEAD(&s->s_inodes);
        INIT_LIST_HEAD(&s->s_dentry_lru);
        init_rwsem(&s->s_umount);
        mutex_init(&s->s_lock);
        lockdep_set_class(&s->s_umount, &type->s_umount_key);
        /*
         * The locking rules for s_lock are up to the
         * filesystem. For example ext3fs has different
         * lock ordering than usbfs:
         */
        lockdep_set_class(&s->s_lock, &type->s_lock_key);
        down_write(&s->s_umount);
        s->s_count = S_BIAS;
        atomic_set(&s->s_active, 1);
        mutex_init(&s->s_vfs_rename_mutex);
        mutex_init(&s->s_dquot.dqio_mutex);
        mutex_init(&s->s_dquot.dqonoff_mutex);
        init_rwsem(&s->s_dquot.dqptr_sem);
        init_waitqueue_head(&s->s_wait_unfrozen);
        s->s_maxbytes = MAX_NON_LFS;
        s->dq_op = sb_dquot_ops;
        s->s_qcop = sb_quotactl_ops;
        s->s_op = &default_op;
        s->s_time_gran = 1000000000;
    }
out:                                                      
	return s;
}


void kill_block_super(struct super_block *sb)                                                               
{
    struct block_device *bdev = sb->s_bdev;
        
    generic_shutdown_super(sb);
    sync_blockdev(bdev);
    close_bdev_excl(bdev);
}


/**
 *  register_filesystem - register a new filesystem
 *  @fs: the file system structure
 *
 *  Adds the file system passed to the list of file systems the kernel
 *  is aware of for mount and other syscalls. Returns 0 on success,
 *  or a negative errno code on an error.
 *
 *  The &struct file_system_type that is passed is linked into the kernel 
 *  structures and must not be freed until the file system has been
 *  unregistered.
 */

int register_filesystem(struct file_system_type * fs)
{
    int res = 0;
    struct file_system_type **p;

    BUG_ON(strchr(fs->name, '.'));
    if (fs->next)
        return -EBUSY;
    INIT_LIST_HEAD(&fs->fs_supers);
    //write_lock(&file_systems_lock);
    p = find_filesystem(fs->name, strlen(fs->name));
    if (*p){
        res = -EBUSY;
    }else{
        *p = fs;//二级指针赋值一级指针的技巧
    }
    //write_unlock(&file_systems_lock);
    return res;
}

/**
 *  unregister_filesystem - unregister a file system
 *  @fs: filesystem to unregister
 *
 *  Remove a file system that was previously successfully registered
 *  with the kernel. An error is returned if the file system is not found.
 *  Zero is returned on a success.
 *  
 *  Once this function has returned the &struct file_system_type structure
 *  may be freed or reused.
 */

int unregister_filesystem(struct file_system_type * fs)
{
    struct file_system_type ** tmp;

    //write_lock(&file_systems_lock);
    tmp = &file_systems;
    while (*tmp) {
        if (fs == *tmp) {
            *tmp = fs->next;
            fs->next = NULL;
            //write_unlock(&file_systems_lock);
            return 0;
        }
        tmp = &(*tmp)->next;
    }
    //write_unlock(&file_systems_lock);
    return -EINVAL;
}


struct file_system_type **find_filesystem(const char *name, unsigned len)
{
    struct file_system_type **p;
    
    for (p = &file_systems; *p; p = &(*p)->next)
        if (strlen((*p)->name) == len &&
            strncmp((*p)->name, name, len) == 0)
            break;
    return p;
}



//格式化设备为ext2
int format_to_ext2(struct virtual_device *dev)
{
	if(INODE_SIZE*INODES_PER_BLOCK!=BLOCK_SIZE){
		cout <<"bad inode size."
			 <<INODE_SIZE
			 <<"*"
			 <<INODES_PER_BLOCK
			 <<"!="
			 <<BLOCK_SIZE
			 <<endl;
	}
	
	ext2_super_block super;
	
	//block
	super.s_blocks_count = BLOCK_NUM;
	super.s_blocks_per_group = BLOCK_NUM;
	super.s_free_blocks_count = BLOCK_NUM;
	
	//inode
	int inodes = super.s_blocks_count / 3;
	inodes=((inodes+INODES_PER_BLOCK -1)& ~(INODES_PER_BLOCK -1));
	super.s_inodes_count = inodes;
	super.s_inodes_per_group = inodes;
	super.s_free_inodes_count = inodes;
	super.s_r_blocks_count = 0;
	super.s_log_block_size = BLOCK_SIZE;
	super.s_log_frag_size = 0;
	super.s_frags_per_group = 0;
	super.s_first_ino = ROOT_INO;
	super.s_inode_size = sizeof(ext2_inode);
	
	
	//first data block
	int inodes_blocks = UPPER(inodes, INODES_PER_BLOCK);
	super.s_first_data_block = 0+1+1+1+inodes_blocks+1;
	
	//dgt
	ext2_group_desc gdt;
	gdt.bg_block_bitmap = 0+1+1;
	gdt.bg_inode_bitmap = 0+1+1+1;
	gdt.bg_inode_table = 0+1+1+1+1;
	gdt.bg_free_blocks_count = BLOCK_NUM;
	gdt.bg_free_inodes_count = inodes;
	gdt.bg_used_dirs_count = 0;
	
	
	//block and inode bitmap
	Bitmap *blockbitmap = new Bitmap(BITS_PER_BLOCK);
	blockbitmap->setall();
	
	for(int i = super.s_first_data_block; i < super.s_blocks_count; ++i){
		blockbitmap->clear(i);
	}
	
	Bitmap *inodebitmap = new Bitmap(BITS_PER_BLOCK);
	inodebitmap->setall();
	for(int i = ROOT_INO; i < super.s_inodes_count; ++i){
		inodebitmap->clear(i);
	}
	
	//inode table
	char inodetable[inodes_blocks * BLOCK_SIZE];
	memset(inodetable, 0, inodes_blocks * BLOCK_SIZE);
	
	//root 
	char rootblock[BLOCK_SIZE];
	memset(rootblock, 0, BLOCK_SIZE);
	ext2_inode *root_inode = (ext2_inode *)inodetable;
	dir_entry *tmp_entry = (dir_entry *)rootblock;
	tmp_entry->ino = ROOT_INO;
	memcpy(tmp_entry->name, ".", sizeof("."));
	tmp_entry->name_len = sizeof("."); 
	tmp_entry++;
	tmp_entry->ino = ROOT_INO;
	memcpy(tmp_entry->name, "..", sizeof(".."));
	tmp_entry->name_len = sizeof("..");
	root_inode[ROOT_INO].i_size = (sizeof(dir_entry))*2;
	root_inode[ROOT_INO].i_blocks = 1;
	root_inode[ROOT_INO].i_block[0] = 7;
	
	super.s_free_blocks_count -=1;
	super.s_free_inodes_count -=1;
	super.s_r_blocks_count +=1;
	gdt.bg_free_blocks_count -=1;
	gdt.bg_free_inodes_count -=1;
	blockbitmap->set(7);
	inodebitmap->set(ROOT_INO);
	
	
	
	
	//write super block
	off_t offset;
	if((offset= lseek(m_DiskFd, 0, SEEK_SET)) != 0){
		cout <<"lseek error"<<endl;
	}
	ssize_t size;
	if((size = write(m_DiskFd, &super, sizeof(super))) < 0){
		cout <<"write error"<<endl;
	}
	
	//write gdt
	if((offset = lseek(m_DiskFd, BLOCK_SIZE, SEEK_SET)) !=	BLOCK_SIZE){
		cout <<"lseek error"<<endl;
	}
	if((size = write(m_DiskFd, &gdt, sizeof(gdt)) < 0)){
		cout <<"write error"<<endl;
	}
	
	//block bitmap
	if((offset = lseek(m_DiskFd, BLOCK_SIZE*2, SEEK_SET)) != BLOCK_SIZE*2){
		cout <<"lseek error"<<endl;
	}
	if((size = write(m_DiskFd, blockbitmap->m_pMap, BLOCK_SIZE)) < 0){
		cout <<"write error"<<endl;
	}
	
	//inode bitmap
	if((offset = lseek(m_DiskFd, BLOCK_SIZE*3, SEEK_SET)) != BLOCK_SIZE*3){
		cout <<"lseek error"<<endl;
	}
	if((size = write(m_DiskFd, inodebitmap->m_pMap, BLOCK_SIZE)) < 0){
		cout <<"write error"<<endl;
	}
	
	//inode table
	if((offset = lseek(m_DiskFd, BLOCK_SIZE*4, SEEK_SET)) != BLOCK_SIZE*4){
		cout <<"lseek error"<<endl;
	}
	if((size = write(m_DiskFd, inodetable, inodes_blocks * BLOCK_SIZE)) < 0){
		cout <<"write error"<<endl;
	}
	//root block
	if((offset = lseek(m_DiskFd, BLOCK_SIZE*7, SEEK_SET)) != BLOCK_SIZE*7){
		cout <<"lseek error"<<endl;
	}
	if((size = write(m_DiskFd, rootblock, BLOCK_SIZE)) < 0){
		cout <<"write error"<<endl;
	}
	

	return 0;
}



loff_t generic_file_llseek(struct file *file, loff_t offset, int origin)                                    
{   
	/*
    loff_t n;
    mutex_lock(&file->f_dentry->d_inode->i_mutex);
    n = generic_file_llseek_unlocked(file, offset, origin);
    mutex_unlock(&file->f_dentry->d_inode->i_mutex);
    return n;   
    */
  return 0;
}

ssize_t do_sync_read(struct file *filp, char __user *buf, size_t len, loff_t *ppos)
{                              
	/*
    struct iovec iov = { .iov_base = buf, .iov_len = len };
    struct kiocb kiocb;
    ssize_t ret;

    init_sync_kiocb(&kiocb, filp);
    kiocb.ki_pos = *ppos;
    kiocb.ki_left = len;

    for (;;) {
        ret = filp->f_op->aio_read(&kiocb, &iov, 1, kiocb.ki_pos);
        if (ret != -EIOCBRETRY)
            break;
        wait_on_retry_sync_kiocb(&kiocb);
    }

    if (-EIOCBQUEUED == ret)
        ret = wait_on_sync_kiocb(&kiocb);
    *ppos = kiocb.ki_pos;
    return ret;
    */

	return 0;
}

ssize_t do_sync_write(struct file *filp, const char __user *buf, size_t len, loff_t *ppos)
{         
	/*
    struct iovec iov = { .iov_base = (void __user *)buf, .iov_len = len };
    struct kiocb kiocb;
    ssize_t ret;

    init_sync_kiocb(&kiocb, filp);
    kiocb.ki_pos = *ppos;
    kiocb.ki_left = len;
    
    for (;;) {  
        ret = filp->f_op->aio_write(&kiocb, &iov, 1, kiocb.ki_pos);
        if (ret != -EIOCBRETRY)
            break;  
        wait_on_retry_sync_kiocb(&kiocb);
    }

    if (-EIOCBQUEUED == ret) 
        ret = wait_on_sync_kiocb(&kiocb);
    *ppos = kiocb.ki_pos;
    return ret; 
    */
	return 0;
} 

/** 
 * generic_file_aio_read - generic filesystem read routine
 * @iocb:   kernel I/O control block
 * @iov:    io vector request
 * @nr_segs:    number of segments in the iovec
 * @pos:    current file position
 *
 * This is the "read()" routine for all filesystems
 * that can use the page cache directly.
 */ 
ssize_t
generic_file_aio_read(struct kiocb *iocb, const struct iovec *iov,
        unsigned long nr_segs, loff_t pos)
{
    struct file *filp = iocb->ki_filp;
    ssize_t retval;
    unsigned long seg;
    size_t count;
    loff_t *ppos = &iocb->ki_pos;

    count = 0;
    retval = generic_segment_checks(iov, &nr_segs, &count, VERIFY_WRITE);
    if (retval)
        return retval;

    /* coalesce the iovecs and go direct-to-BIO for O_DIRECT */
    if (filp->f_flags & O_DIRECT) {
        loff_t size;
        struct address_space *mapping;
        struct inode *inode;

        mapping = filp->f_mapping;
        inode = mapping->host;
        if (!count)
            goto out; /* skip atime */
        size = i_size_read(inode);
        if (pos < size) {
            retval = filemap_write_and_wait(mapping);
            if (!retval) {
                retval = mapping->a_ops->direct_IO(READ, iocb,
                            iov, pos, nr_segs);
            }
            if (retval > 0)
                *ppos = pos + retval;
            if (retval) {
                file_accessed(filp);
                goto out;
            }
        }
    }
	
	for (seg = 0; seg < nr_segs; seg++) {
		read_descriptor_t desc;

		desc.written = 0;
		desc.arg.buf = iov[seg].iov_base;
		desc.count = iov[seg].iov_len;
		if (desc.count == 0)
			continue;
		desc.error = 0;
		do_generic_file_read(filp, ppos, &desc, file_read_actor);
		retval += desc.written;
		if (desc.error) {
			retval = retval ?: desc.error;
			break;
		}
		if (desc.count > 0)
			break;
	}
out:
	return retval;
}

ssize_t generic_file_aio_write(struct kiocb *iocb, const struct iovec *iov,
        unsigned long nr_segs, loff_t pos)
{
	/*
    struct file *file = iocb->ki_filp;
    struct address_space *mapping = file->f_mapping;
    struct inode *inode = mapping->host;
    ssize_t ret;
    
    BUG_ON(iocb->ki_pos != pos);
    
    mutex_lock(&inode->i_mutex);
    ret = __generic_file_aio_write_nolock(iocb, iov, nr_segs,
            &iocb->ki_pos);
    mutex_unlock(&inode->i_mutex);

    if (ret > 0 && ((file->f_flags & O_SYNC) || IS_SYNC(inode))) {
        ssize_t err;
    
        err = sync_page_range(inode, mapping, pos, ret);
        if (err < 0)
            ret = err;
    }
    return ret;
    */
	return 0;
} 

/* This is used for a general mmap of a disk file */
    
int generic_file_mmap(struct file * file, struct vm_area_struct * vma)                                                                                                                                                           
{   
/*
    struct address_space *mapping = file->f_mapping;
    
    if (!mapping->a_ops->readpage)
        return -ENOEXEC;
    file_accessed(file);
    vma->vm_ops = &generic_file_vm_ops;
    vma->vm_flags |= VM_CAN_NONLINEAR;
    */
    return 0;
}

/*
 * Called when an inode is about to be open.
 * We use this to disallow opening large files on 32bit systems if
 * the caller didn't specify O_LARGEFILE.  On 64bit systems we force
 * on this flag in sys_open.
 */ 
int generic_file_open(struct inode * inode, struct file * filp)
{                                                                                                                                                                                                                                
    if (!(filp->f_flags & O_LARGEFILE) && i_size_read(inode) > MAX_NON_LFS)
        return -EOVERFLOW;
    return 0;
}

/*      
 * Called when filp is released. This happens when all file descriptors
 * for a single struct file are closed. Note that different open() calls
 * for the same file yield different struct file structures.
 */
static int ext2_release_file (struct inode * inode, struct file * filp)                                                                                                                                                          
{

    if (filp->f_mode & FMODE_WRITE) {
        mutex_lock(&EXT2_I(inode)->truncate_mutex);
        ext2_discard_reservation(inode);
        mutex_unlock(&EXT2_I(inode)->truncate_mutex);
    }
    
    return 0;
}

/*      
 *  File may be NULL when we are called. Perhaps we shouldn't
 *  even pass file to fsync ?
 */

int ext2_sync_file(struct file *file, struct dentry *dentry, int datasync)
{
/*
    struct inode *inode = dentry->d_inode;
    int err;
    int ret;

    ret = sync_mapping_buffers(inode->i_mapping);
    if (!(inode->i_state & I_DIRTY))
        return ret;
    if (datasync && !(inode->i_state & I_DIRTY_DATASYNC))
        return ret;

    err = ext2_sync_inode(inode);
    if (ret == 0)
        ret = err;
    return ret;
    */
	return 0;
} 

long ext2_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	//TODO
	return 0;
}








/*
 * We have mostly NULL's here: the current defaults are ok for
 * the ext2 filesystem.
 */ 
const struct file_operations ext2_file_operations = {
    .llseek     = generic_file_llseek,
    .read       = do_sync_read,
    .write      = do_sync_write,
    .aio_read   = generic_file_aio_read,	//must
    .aio_write  = generic_file_aio_write,
    .unlocked_ioctl = ext2_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl   = ext2_compat_ioctl,
#endif
    .mmap       = generic_file_mmap,
    .open       = generic_file_open,		//must
    .release    = ext2_release_file,		//must
    .fsync      = ext2_sync_file,
    .splice_read    = NULL,
    .splice_write   = NULL,
};



