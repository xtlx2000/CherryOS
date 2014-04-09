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
                         0, (SLAB_RECLAIM_ACCOUNT|
                        SLAB_MEM_SPREAD),
                         init_once);
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
    struct file_system_type ** p;

    BUG_ON(strchr(fs->name, '.'));
    if (fs->next)
        return -EBUSY;
    INIT_LIST_HEAD(&fs->fs_supers);
    //write_lock(&file_systems_lock);
    p = find_filesystem(fs->name, strlen(fs->name));
    if (*p)
        res = -EBUSY;
    else
        *p = fs;
    //write_unlock(&file_systems_lock);
    return res;
}


struct file_system_type **find_filesystem(const char *name, unsigned len)
{
    struct file_system_type **p;
    for (p=&file_systems; *p; p=&(*p)->next)
        if (strlen((*p)->name) == len &&
            strncmp((*p)->name, name, len) == 0)
            break;
    return p;
}

