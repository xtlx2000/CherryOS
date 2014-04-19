#include "fs.h"
#include "ext2.h"
#include "device.h"
#include "atomic.h"

int fs_init()
{
	// 1 vfs 
	vfs_init();
	// 2 real fs register
	init_ext2_fs();

	// 3 real device detect, we use sda and format it to ext2
	device_init();

	// 4 format sda to ext2
	ret = format_to_ext2(&device_sda);

	// 5 mount sda and prepare data
	mount(&device_sda, "ext2Mounted");
	mkdir("/ext2Mounted/bar");
	int fd = open("/ext2Mounted/foo.txt");
	write(fd, "hello world.", 13);
	close(fd);
	
	
	
	return 0;
}

//OK
long sys_mount(char *dev_name, char *dir_name, char *type, unsigned long flags, void *data)
{
	struct ext2_super_block *super;
	char *buffer;

	super = (struct ext2_super_block *)malloc(sizeof(struct ext2_super_block));
	if(super == NULL){
		free(super);
		return NULL;
	}

	//根据device从磁盘读取super block
	ext2_get_super(dev_name, super);
	if(super->s_magic != EXT2_SUPER_MAGIC){
		free(super);
		return NULL;
	}

	//入队
	struct file_system_type **p;

    //write_lock(&file_systems_lock);
    p = find_filesystem("ext2", sizeof("ext2"));
    if (*p){//found
        list_add(&super->filesystem_type, &(*p->fs_supers));
    }else{//not found
        return -1;
    }
    //write_unlock(&file_systems_lock);

	//write topDirs
	memset(topDirs[DirsNum].dev_name, 0, MAX_DEVICE_NAMELEN);
	memcpy(topDirs[DirsNum].dev_name, dir_name, strlen(dir_name));
	topDirs[DirsNum].super_block = super;
	DirsNum++;

	
	current_sb = super;

	return 0;
	
}

//OK
int ext2_get_super(char *dev_name, struct ext2_super_block *super)
{	
	
	struct ext2_super_block *read = NULL;
	struct virtual_device *dev_pos = NULL;
	struct virtual_device *dev = NULL;


	list_for_each_entry(dev_pos, &device_list, dev_list){
		if(strcmp(dev_pos->name, dev_name) == 0){
			dev = dev_pos;
			break;
		}
	}

	if(!dev){
		return -1;
	}

	read = (struct ext2_super_block *)dev;
	
	//NOTE: I want to persistent the super to disk soon, so i write code like below
	super->s_inodes_count = read->s_inodes_count;			//inode数目//
	super->s_blocks_count = read->s_blocks_count;			//block数目//
	super->s_r_blocks_count = read->s_r_blocks_count;		//已分配的块数目//
	super->s_free_blocks_count = read->s_free_blocks_count;	//空闲块数目//
	super->s_free_inodes_count = read->s_free_inodes_count;	//空闲inode数目//
	super->s_first_data_block = read->s_first_data_block;		//第一个数据块//
	super->s_log_block_size = read->s_log_block_size;		//块长度//
	super->s_log_frag_size = read->s_log_frag_size;		//碎片长度//
	super->s_blocks_per_group = read->s_blocks_per_group;		//每个快组包含的快数目//
	super->s_frags_per_group = read->s_frags_per_group;		//每个快组包含的碎片//
	super->s_inodes_per_group = read->s_inodes_per_group;		//每个快组的inode数目//
	super->s_state = read->s_state;				//zhuangtai

	super->s_first_ino = read->s_first_ino;			//第一个非保留的inode
	super->s_inode_size = read->s_inode_size;			//inode结构的大小
	
	super->s_magic = read->s_magic;

	super->dev = read->dev;


	//list_head ignore

	return 0;
	
	
}




struct ext2_DIR *sys_opendir(const char *name)
{
	struct ext2_DIR *dir;
	int ino;

	struct ext2_inode *inode;
	int ret;

	ino = ext2_seek_name(name);

	if(ino == 0){
		return NULL;
	}

	inode = (struct ext2_inode*)malloc(sizeof(struct ext2_inode));
	if(inode == NULL){
		return NULL;
	}

	ret = ext2_get_inode(ino, inode);
	if(ret == -1){
		free(inode);
		return NULL;
	}

	if(!S_ISDIR(inode->i_mode)){
		free(inode);
		return NULL;
	}

	dir = (struct ext2_DIR*)malloc(sizeof(struct ext2_DIR));
	if(dir == NULL){
		free(inode);
		return NULL;
	}

	//dir
}



unsigned int ext2_seek_name(const char *name)
{
	struct ext2_inode inode;
	struct ext2_super_block *super = NULL;
	int ret;
	unsigned int ino;
	off_t index;
	struct ext2_dir_entry_2 entry;

	ino = EXT2_ROOT_INO;
	//循环处理每个路径分量
	while(1) {
		while (*name == '/')
			name++;
		if (!*name)
		    break;

		int i;
		for(i = 0; i < TOPDIRSNUM; i++){
			if(strcmp(topDirs[i].topdir, name) == 0){
				super = topDirs[i].super_block;
			}
		}

		if(!super){
			return -1;
		}
		    
		ret = ext2_get_inode(super, ino, &inode);
		if (ret == -1)
			return 0;
		index = 0;
		//循环读取每个路径分量(目录)的所有dir_entry条目
		//逐个跟name比较，如果相同，则记录ino,跳出二级循环
		while (1) {
			index = ext2_dir_entry(&inode, index, &entry);
			if (index == -1)
				return 0;
			ret = strncmp(name, entry.name, entry.name_len);
			if (ret == 0  &&
			    (name[entry.name_len] == 0 ||
			     name[entry.name_len] == '/')) {
			     	ino = entry.inode;
				break;
			}
		}
		name += entry.name_len;
	}

	return ino;//最终的这个ino就是最后一级目录的ino
}


off_t ext2_dir_entry(struct ext2_inode *inode,
		     off_t index, struct ext2_dir_entry_2 *entry)
{
	int ret;

	ret = ext2_read_data(inode, index,
			     (char*)entry, sizeof(*entry));
	if (ret == -1)
		return -1;

        entry->inode = __le32_to_cpu(entry->inode);
        entry->rec_len = __le16_to_cpu(entry->rec_len);
	return index + entry->rec_len;
}



int ext2_read_data(struct ext2_inode *inode,
		   off_t offset, char *buffer, size_t length)
{
	unsigned int logical, physical;
	int blocksize = EXT2_BLOCK_SIZE(super_block);
	int shift;
	size_t read;

	if (offset >= inode->i_size)
		return -1;

	if (offset + length >= inode->i_size)
		length = inode->i_size - offset;

	read = 0;
	logical = offset / blocksize;
	shift = offset % blocksize;

	if (shift) {
		physical = ext2_get_block_addr(volume, inode, logical);
		ext2_read_block(volume, physical);

		if (length < blocksize - shift) {
			memcpy(buffer, volume->buffer + shift, length);
			return length;
		}
		read += blocksize - shift;
		memcpy(buffer, volume->buffer + shift, read);

		buffer += read;
		length -= read;
		logical++;
	}

	while (length) {
		physical = ext2_get_block_addr(volume, inode, logical);
		ext2_read_block(volume, physical);

		if (length < blocksize) {
			memcpy(buffer, volume->buffer, length);
			read += length;
			return read;
		}
		memcpy(buffer, volume->buffer, blocksize);

		buffer += blocksize;
		length -= blocksize;
		read += blocksize;
		logical++;
	}

	return read;
}



unsigned int ext2_get_block_addr(ext2_VOLUME* volume, struct ext2_inode *inode,
				 unsigned int logical)
{
	unsigned int physical;
	unsigned int addr_per_block;

	/* direct */

	if (logical < EXT2_NDIR_BLOCKS) {
		physical = inode->i_block[logical];
		return physical;
	}

	/* indirect */

	logical -= EXT2_NDIR_BLOCKS;

	addr_per_block = EXT2_ADDR_PER_BLOCK (volume->super);
	if (logical < addr_per_block) {
		ext2_read_block(volume, inode->i_block[EXT2_IND_BLOCK]);
		physical = __le32_to_cpu(((unsigned int *)volume->buffer)[logical]);
		return physical;
	}

	/* double indirect */

	logical -=  addr_per_block;

	if (logical < addr_per_block * addr_per_block) {
		ext2_read_block(volume, inode->i_block[EXT2_DIND_BLOCK]);
		physical = __le32_to_cpu(((unsigned int *)volume->buffer)
						[logical / addr_per_block]);
		ext2_read_block(volume, physical);
		physical = __le32_to_cpu(((unsigned int *)volume->buffer)
						[logical % addr_per_block]);
		return physical;
	}

	/* triple indirect */

	logical -= addr_per_block * addr_per_block;
	ext2_read_block(volume, inode->i_block[EXT2_DIND_BLOCK]);
	physical = __le32_to_cpu(((unsigned int *)volume->buffer)
				[logical / (addr_per_block * addr_per_block)]);
	ext2_read_block(volume, physical);
	logical = logical % (addr_per_block * addr_per_block);
	physical = __le32_to_cpu(((unsigned int *)volume->buffer)[logical / addr_per_block]);
	ext2_read_block(volume, physical);
	physical = __le32_to_cpu(((unsigned int *)volume->buffer)[logical % addr_per_block]);
	return physical;
}

void ext2_read_block(ext2_VOLUME* volume, unsigned int fsblock)
{
	llong offset;

	if (fsblock == volume->current)
		return;

	volume->current = fsblock;
	offset = fsblock * EXT2_BLOCK_SIZE(volume->super);

	seek_io(volume->fd, offset);
	read_io(volume->fd, volume->buffer, EXT2_BLOCK_SIZE(volume->super));
}



void ext2_get_group_desc(struct ext2_super_block super,
		   int group_id, struct ext2_group_desc *gdp)
{
	unsigned int block, offset;
	struct ext2_group_desc *le_gdp;

	block = 1 + super->s_first_data_block;
	block += group_id / EXT2_DESC_PER_BLOCK(super);
	ext2_read_block(volume,  block);

	offset = group_id % EXT2_DESC_PER_BLOCK(super);
	offset *= sizeof(*gdp);

	le_gdp = (struct ext2_group_desc *)(volume->buffer + offset);

	gdp->bg_block_bitmap = __le32_to_cpu(le_gdp->bg_block_bitmap);
	gdp->bg_inode_bitmap = __le32_to_cpu(le_gdp->bg_inode_bitmap);
	gdp->bg_inode_table = __le32_to_cpu(le_gdp->bg_inode_table);
	gdp->bg_free_blocks_count = __le16_to_cpu(le_gdp->bg_free_blocks_count);
	gdp->bg_free_inodes_count = __le16_to_cpu(le_gdp->bg_free_inodes_count);
	gdp->bg_used_dirs_count = __le16_to_cpu(le_gdp->bg_used_dirs_count);
}




int ext2_get_inode(struct ext2_super_block *super, unsigned int ino, struct ext2_inode *inode)
{
	struct ext2_group_desc desc;
	unsigned int block;
	unsigned int group_id;
	unsigned int offset;
	struct ext2_inode *le_inode;
	int i;

	ino--;

	group_id = ino / EXT2_INODES_PER_GROUP(super);
	ext2_get_group_desc(super, group_id, &desc);

	ino %= EXT2_INODES_PER_GROUP(super);

	block = desc.bg_inode_table;
	block += ino / (EXT2_BLOCK_SIZE(super) /
			EXT2_INODE_SIZE(super));
	ext2_read_block(block);

	offset = ino % (EXT2_BLOCK_SIZE(super) /
			EXT2_INODE_SIZE(super));
	offset *= EXT2_INODE_SIZE(super);

	le_inode = (struct ext2_inode *)(volume->buffer + offset);

	inode->i_mode = __le16_to_cpu(le_inode->i_mode);
	inode->i_uid = __le16_to_cpu(le_inode->i_uid);
	inode->i_size = __le32_to_cpu(le_inode->i_size);
	inode->i_atime = __le32_to_cpu(le_inode->i_atime);
	inode->i_ctime = __le32_to_cpu(le_inode->i_ctime);
	inode->i_mtime = __le32_to_cpu(le_inode->i_mtime);
	inode->i_dtime = __le32_to_cpu(le_inode->i_dtime);
	inode->i_gid = __le16_to_cpu(le_inode->i_gid);
	inode->i_links_count = __le16_to_cpu(le_inode->i_links_count);
	inode->i_blocks = __le32_to_cpu(le_inode->i_blocks);
	inode->i_flags = __le32_to_cpu(le_inode->i_flags);
	if (S_ISLNK(inode->i_mode)) {
		memcpy(inode->i_block, le_inode->i_block, EXT2_N_BLOCKS * 4);
	} else {
		for (i = 0; i < EXT2_N_BLOCKS; i++)
			inode->i_block[i] = __le32_to_cpu(le_inode->i_block[i]);
        }
	inode->i_generation = __le32_to_cpu(le_inode->i_generation);
	inode->i_file_acl = __le32_to_cpu(le_inode->i_file_acl);
	inode->i_dir_acl = __le32_to_cpu(le_inode->i_dir_acl);
	inode->i_faddr = __le32_to_cpu(le_inode->i_faddr);
	inode->osd2.linux2.l_i_frag = le_inode->osd2.linux2.l_i_frag;
	inode->osd2.linux2.l_i_fsize = le_inode->osd2.linux2.l_i_fsize;
	inode->osd2.linux2.l_i_uid_high =
			__le16_to_cpu(le_inode->osd2.linux2.l_i_uid_high);
	inode->osd2.linux2.l_i_gid_high =
			__le16_to_cpu(le_inode->osd2.linux2.l_i_gid_high);
	return 0;
}





#define AT_FDCWD        (-100)    /* Special value used to indicate
                                           openat should use the current
                                           working directory. */


long sys_open(const char *filename, int flags, int mode)                                  
{   
    long ret;
    
    //ret = do_sys_open( AT_FDCWD, filename, flags, mode);
    /* avoid REGPARM breakage on x86: */
    //asmlinkage_protect(3, ret, filename, flags, mode);


	ext2_FILE *file;
	struct ext2_inode *inode;
	int ino;
	int ret;

	ino = ext2_seek_name(volume, pathname);
	if (ino == 0)
		return NULL;

	inode = (struct ext2_inode*)malloc(sizeof(struct ext2_inode));
	if (inode == NULL)
		return NULL;

	ret = ext2_get_inode(volume, ino, inode);
	if (ret == -1) {
		free(inode);
		return NULL;
	}
	if (S_ISLNK(inode->i_mode)) {
		static char buffer[1024];
		int i, last = 0;
		strcpy(buffer, pathname);
		for (i = 0; buffer[i]; i++)
			if (buffer[i] == '\\')
				last = i;
		buffer[last] = '\\';
		strcpy(buffer + last + 1, (char*)inode->i_block);
		ino = ext2_seek_name((ext2_VOLUME*)volume, buffer);
		if (ino == 0) {
			free(inode);
			return NULL;
		}
		ret = ext2_get_inode((ext2_VOLUME*)volume, ino, inode);
		if (ret == -1) {
			free(inode);
			return NULL;
		}
	}

	file = (ext2_FILE*)malloc(sizeof(ext2_FILE));
	if (file == NULL) {
		free(inode);
		return NULL;
	}
	file->volume = volume;
	file->inode = inode;
	file->offset = 0;
	file->path = strdup(pathname);
    
    return ret;
}


//查找一个未使用的文件描述符
#define get_unused_fd_flags(flags) alloc_fd(0, (flags))


long do_sys_open(int dfd, const char *filename, int flags, int mode)
{                                                                                                           
    char *tmp = filename;
    int fd = 0;
        
    //if (!IS_ERR(tmp)) {
    //查找一个未使用的文件描述符
    fd = get_unused_fd_flags(flags);
    if (fd >= 0) {
    	//查找文件的inode
        struct file *f = do_filp_open(dfd, tmp, flags, mode);
        if (IS_ERR(f)) {
            put_unused_fd(fd);
            fd = PTR_ERR(f);
        } else {
            fsnotify_open(f->f_path.dentry);
            fd_install(fd, f);
        }
    }
    //putname(tmp);
    //}
    return fd;
}



/*  
 * Note that while the flag value (low two bits) for sys_open means:
 *  00 - read-only
 *  01 - write-only
 *  10 - read-write
 *  11 - special
 * it is changed into
 *  00 - no permissions needed
 *  01 - read-permission
 *  10 - write-permission
 *  11 - read-write
 * for the internal routines (ie open_namei()/follow_link() etc)
 * This is more logical, and also allows the 00 "no perm needed"
 * to be used for symlinks (where the permissions are checked
 * later).
 *
*/
static inline int open_to_namei_flags(int flag)
{
    if ((flag+1) & O_ACCMODE)
        flag++;
    return flag;
}  




/*  
 * Note that the low bits of the passed in "open_flag"
 * are not the same as in the local variable "flag". See
 * open_to_namei_flags() for more details.
 */                
 //借助两个辅助函数来查找文件的inode：
 //1、open_namei调用path_lookup函数查找inode并执行几个额外的检查
 //（例如，确定应用程序是否试图打开目录，然后像普通文件一样处理）。
 //如果需要创建新的文件系统项，该函数还需要应用存储在进程umask（current->fs->umask）
 //中的权限位的默认设置。
 //2、nameidata_to_filp初始化预读结构，将新创建的file实例放置到超级块的s_files链表上，
 //并调用底层文件系统的file_operations结构中的open函数。
struct file *do_filp_open(int dfd, const char *pathname,
        int open_flag, int mode)
{           
    struct file *filp;
    struct nameidata nd;
    int acc_mode, error;
    struct path path;
    struct dentry *dir;
    int count = 0;
    int will_write;
    int flag = open_to_namei_flags(open_flag);
    
    //acc_mode = MAY_OPEN | ACC_MODE(flag);

    /* O_TRUNC implies we need access checks for write permissions */
    //if (flag & O_TRUNC)
    //    acc_mode |= MAY_WRITE;

    /* Allow the LSM permission hook to distinguish append 
       access from general write access. */
    //if (flag & O_APPEND)
    //    acc_mode |= MAY_APPEND;

    /*
     * The simplest case - just a plain lookup.
     */
    if (!(flag & O_CREAT)) {
        error = path_lookup_open(dfd, pathname, lookup_flags(flag),
                     &nd, flag);
        if (error)
            return ERR_PTR(error);
        goto ok;
    }

    /*
     * Create - we need to know the parent.     
     */
    error = path_lookup_create(dfd, pathname, LOOKUP_PARENT,
                   &nd, flag, mode);
    if (error)
        return ERR_PTR(error);

    /*
     * We have the parent and last component. First of all, check
     * that we are not asked to creat(2) an obvious directory - that
     * will not do.
     */
    error = -EISDIR;
    if (nd.last_type != LAST_NORM || nd.last.name[nd.last.len])
        goto exit;

    dir = nd.path.dentry;
    nd.flags &= ~LOOKUP_PARENT;
    mutex_lock(&dir->d_inode->i_mutex);
    path.dentry = lookup_hash(&nd);
    path.mnt = nd.path.mnt;

do_last:
    error = PTR_ERR(path.dentry);
    if (IS_ERR(path.dentry)) {
        mutex_unlock(&dir->d_inode->i_mutex);
        goto exit;
    }

    if (IS_ERR(nd.intent.open.file)) {
        error = PTR_ERR(nd.intent.open.file);
        goto exit_mutex_unlock;
    }

    /* Negative dentry, just create the file */
    if (!path.dentry->d_inode) {
	        /*
	         * This write is needed to ensure that a
	         * ro->rw transition does not occur between
	         * the time when the file is created and when
		 * a permanent write count is taken through
		 * the 'struct file' in nameidata_to_filp().
		 */
		error = mnt_want_write(nd.path.mnt);
		if (error)
			goto exit_mutex_unlock;
		error = __open_namei_create(&nd, &path, flag, mode);
		if (error) {
			mnt_drop_write(nd.path.mnt);
			goto exit;
		}
		filp = nameidata_to_filp(&nd, open_flag);// second task!
		mnt_drop_write(nd.path.mnt);
		return filp;
	}
    /*
     * It already exists.
     */
    mutex_unlock(&dir->d_inode->i_mutex);
    audit_inode(pathname, path.dentry);

    error = -EEXIST;
    if (flag & O_EXCL)
        goto exit_dput;

    if (__follow_mount(&path)) {
        error = -ELOOP;
        if (flag & O_NOFOLLOW)
            goto exit_dput;
    }

    error = -ENOENT;
    if (!path.dentry->d_inode)
        goto exit_dput;
    if (path.dentry->d_inode->i_op && path.dentry->d_inode->i_op->follow_link)
        goto do_link;

    path_to_nameidata(&path, &nd);
    error = -EISDIR;
    if (path.dentry->d_inode && S_ISDIR(path.dentry->d_inode->i_mode))
        goto exit;

ok:
    /*
     * Consider:
     * 1. may_open() truncates a file
     * 2. a rw->ro mount transition occurs
     * 3. nameidata_to_filp() fails due to
     *    the ro mount.
     * That would be inconsistent, and should
     * be avoided. Taking this mnt write here
     * ensures that (2) can not occur.
     */
    will_write = open_will_write_to_fs(flag, nd.path.dentry->d_inode);
    if (will_write) {
        error = mnt_want_write(nd.path.mnt);
        if (error)
            goto exit;
    }
    error = may_open(&nd, acc_mode, flag);
    if (error) {
        if (will_write)
            mnt_drop_write(nd.path.mnt);
        goto exit;
    }
    filp = nameidata_to_filp(&nd, open_flag);
    /*
     * It is now safe to drop the mnt write
     * because the filp has had a write taken
     * on its behalf.
     */
    if (will_write)
        mnt_drop_write(nd.path.mnt);
    return filp;

exit_mutex_unlock:
    mutex_unlock(&dir->d_inode->i_mutex);
exit_dput:
    path_put_conditional(&path, &nd);
exit:
    if (!IS_ERR(nd.intent.open.file))
        release_open_intent(&nd);
    path_put(&nd.path);
    return ERR_PTR(error);

do_link:
    error = -ELOOP;
    if (flag & O_NOFOLLOW)
        goto exit_dput;
    /*
     * This is subtle. Instead of calling do_follow_link() we do the
     * thing by hands. The reason is that this way we have zero link_count
     * and path_walk() (called from ->follow_link) honoring LOOKUP_PARENT.
     * After that we have the parent and last component, i.e.
     * we are in the same situation as after the first path_walk().
     * Well, almost - if the last component is normal we get its copy
     * stored in nd->last.name and we will have to putname() it when we
     * are done. Procfs-like symlinks just set LAST_BIND.
     */
    nd.flags |= LOOKUP_PARENT;
    error = security_inode_follow_link(path.dentry, &nd);
    if (error)
        goto exit_dput;
    error = __do_follow_link(&path, &nd);
    if (error) {
        /* Does someone understand code flow here? Or it is only
         * me so stupid? Anathema to whoever designed this non-sense
         * with "intent.open".
         */
        release_open_intent(&nd);
        return ERR_PTR(error);

    }
    nd.flags &= ~LOOKUP_PARENT;
    if (nd.last_type == LAST_BIND)
        goto ok;
    error = -EISDIR;
    if (nd.last_type != LAST_NORM)
        goto exit;
    if (nd.last.name[nd.last.len]) {
        __putname(nd.last.name);
        goto exit;
    }
    error = -ELOOP;
    if (count++==32) {
        __putname(nd.last.name);
        goto exit;
    }
    dir = nd.path.dentry;
    mutex_lock(&dir->d_inode->i_mutex);
    path.dentry = lookup_hash(&nd);
    path.mnt = nd.path.mnt;
    __putname(nd.last.name);
    goto do_last;
}




        

	

/*          
 * Restricted form of lookup. Doesn't follow links, single-component only,
 * needs parent already locked. Doesn't follow mounts.
 * SMP-safe.
 */             
static struct dentry *lookup_hash(struct nameidata *nd)                                                     
{               
    int err;

    //err = inode_permission(nd->path.dentry->d_inode, MAY_EXEC);
    //if (err)
    //    return ERR_PTR(err);
    return __lookup_hash(&nd->last, nd->path.dentry, nd);
}

static struct dentry *__lookup_hash(struct qstr *name,
        struct dentry *base, struct nameidata *nd)
{
    struct dentry *dentry;
    struct inode *inode;
    int err;

    inode = base->d_inode;

    /*
     * See if the low-level filesystem might want
     * to use its own hash..
     */
    if (base->d_op && base->d_op->d_hash) {
        err = base->d_op->d_hash(base, name);
        dentry = ERR_PTR(err);
        if (err < 0)
            goto out;
    }

    dentry = cached_lookup(base, name, nd);
    if (!dentry) {
        struct dentry *new;

        /* Don't create child dentry for a dead directory. */
        dentry = ERR_PTR(-ENOENT);
        if (IS_DEADDIR(inode))
            goto out;

        new = d_alloc(base, name);
        dentry = ERR_PTR(-ENOMEM);
        if (!new)
            goto out;
        dentry = inode->i_op->lookup(inode, new, nd);
        if (!dentry)
            dentry = new;
        else
            dput(new);
    }                            

out:
    return dentry;
}



/*  
 * Intent data
 */     
#define LOOKUP_OPEN     (0x0100)
#define LOOKUP_CREATE       (0x0200)


/**
 * path_lookup_open - lookup a file path with open intent
 * @dfd: the directory to use as base, or AT_FDCWD
 * @name: pointer to file name
 * @lookup_flags: lookup intent flags
 * @nd: pointer to nameidata
 * @open_flags: open intent flags
 */
int path_lookup_open(int dfd, const char *name, unsigned int lookup_flags,
        struct nameidata *nd, int open_flags)
{
    return __path_lookup_intent_open(dfd, name, lookup_flags, nd,
            open_flags, 0);
}

/**
 * path_lookup_create - lookup a file path with open + create intent
 * @dfd: the directory to use as base, or AT_FDCWD
 * @name: pointer to file name
 * @lookup_flags: lookup intent flags
 * @nd: pointer to nameidata
 * @open_flags: open intent flags
 * @create_mode: create intent flags
 */
static int path_lookup_create(int dfd, const char *name,
                  unsigned int lookup_flags, struct nameidata *nd,
                  int open_flags, int create_mode)
{
    return __path_lookup_intent_open(dfd, name, lookup_flags|LOOKUP_CREATE,
            nd, open_flags, create_mode);
}


static int __path_lookup_intent_open(int dfd, const char *name,
        unsigned int lookup_flags, struct nameidata *nd,
        int open_flags, int create_mode)
{
    struct file *filp = get_empty_filp();
    int err;

    if (filp == NULL)
        return -ENFILE;
    nd->intent.open.file = filp;
    nd->intent.open.flags = open_flags;
    nd->intent.open.create_mode = create_mode;
    err = do_path_lookup(dfd, name, lookup_flags|LOOKUP_OPEN, nd);
    if (IS_ERR(nd->intent.open.file)) {
        if (err == 0) {
            err = PTR_ERR(nd->intent.open.file);
            path_put(&nd->path);
        }
    } else if (err != 0)
        release_open_intent(nd);
    return err;
}






/* Find an unused file structure and return a pointer to it.
 * Returns NULL, if there are no more free file structures or
 * we run out of memory.
 *
 * Be very careful using this.  You are responsible for
 * getting write access to any mount that you might assign
 * to this filp, if it is opened for write.  If this is not
 * done, you will imbalance int the mount's writer count
 * and a warning at __fput() time.
 */
struct file *get_empty_filp(void)
{
    struct task_struct *tsk;
    static int old_max;
    struct file * f;

    /*
     * Privileged users can go above max_files
     */
    if (get_nr_files() >= files_stat.max_files && !capable(CAP_SYS_ADMIN)) {
        /*
         * percpu_counters are inaccurate.  Do an expensive check before
         * we go and fail.
         */
        if (percpu_counter_sum_positive(&nr_files) >= files_stat.max_files)
            goto over;
    }

    f = kmem_cache_zalloc(filp_cachep, GFP_KERNEL);
    if (f == NULL)
        goto fail;

    percpu_counter_inc(&nr_files);
    if (security_file_alloc(f))
        goto fail_sec;

    tsk = current;
    INIT_LIST_HEAD(&f->f_u.fu_list);
    atomic_long_set(&f->f_count, 1);
    rwlock_init(&f->f_owner.lock);
    f->f_uid = tsk->fsuid;
    f->f_gid = tsk->fsgid;
    eventpoll_init_file(f);
    /* f->f_version: 0 */
    return f;

over:
    /* Ran out of filps - report that */
    if (get_nr_files() > old_max) {
        printk(KERN_INFO "VFS: file-max limit %d reached\n",
                    get_max_files());                                
		old_max = get_nr_files();
	}
	goto fail;

fail_sec:
	file_free(f);
fail:
	return NULL;
}


/*      
 * Type of the last component on LOOKUP_PARENT
 */ 
enum {LAST_NORM, LAST_ROOT, LAST_DOT, LAST_DOTDOT, LAST_BIND}; 


#define S_IFMT  00170000
#define S_IFSOCK 0140000
#define S_IFLNK  0120000
#define S_IFREG  0100000
#define S_IFBLK  0060000
#define S_IFDIR  0040000
#define S_IFCHR  0020000
#define S_IFIFO  0010000
#define S_ISUID  0004000
#define S_ISGID  0002000
#define S_ISVTX  0001000



#define S_ISLNK(m)  (((m) & S_IFMT) == S_IFLNK)
#define S_ISREG(m)  (((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m)  (((m) & S_IFMT) == S_IFDIR)                                                             
#define S_ISCHR(m)  (((m) & S_IFMT) == S_IFCHR)
#define S_ISBLK(m)  (((m) & S_IFMT) == S_IFBLK)
#define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)
#define S_ISSOCK(m) (((m) & S_IFMT) == S_IFSOCK)




/* Returns 0 and nd will be valid on success; Retuns error, otherwise. */
static int do_path_lookup(int dfd, const char *name,
                unsigned int flags, struct nameidata *nd)
{
    int retval = 0;
    int fput_needed;
    struct file *file;
    struct fs_struct *fs = current->fs;

    nd->last_type = LAST_ROOT; /* if there are only slashes... */
    nd->flags = flags;
    nd->depth = 0;

    if (*name=='/') {
        //read_lock(&fs->lock);
        nd->path = fs->root;
        path_get(&fs->root);
        //read_unlock(&fs->lock);
    } else if (dfd == AT_FDCWD) {
        //read_lock(&fs->lock);
        nd->path = fs->pwd;
        path_get(&fs->pwd);
        //read_unlock(&fs->lock);
    } else {
        struct dentry *dentry;

        file = fget_light(dfd, &fput_needed);
        retval = -EBADF;
        if (!file)
            goto out_fail;

        dentry = file->f_path.dentry;

        retval = -ENOTDIR;
        if (!S_ISDIR(dentry->d_inode->i_mode))
            goto fput_fail;

        retval = file_permission(file, MAY_EXEC);
        if (retval)
            goto fput_fail;                     

		nd->path = file->f_path;
		path_get(&file->f_path);

		fput_light(file, fput_needed);
	}

	retval = path_walk(name, nd);
	if (unlikely(!retval && !audit_dummy_context() && nd->path.dentry &&
				nd->path.dentry->d_inode))
		audit_inode(name, nd->path.dentry);
out_fail:
	return retval;

fput_fail:
	fput_light(file, fput_needed);
	goto out_fail;
}



int path_walk(const char *name, struct nameidata *nd)                                                
{
    current->total_link_count = 0;
    return link_path_walk(name, nd);
}



/*
 * Wrapper to retry pathname resolution whenever the underlying
 * file system returns an ESTALE.
 *
 * Retry the whole path once, forcing real lookup requests
 * instead of relying on the dcache.
 */
int link_path_walk(const char *name, struct nameidata *nd)
{
    struct path save = nd->path;
    int result;

    /* make sure the stuff we saved doesn't go away */
    path_get(&save);

    result = __link_path_walk(name, nd);
    if (result == -ESTALE) {
        /* nd->path had been dropped */
        nd->path = save;
        path_get(&nd->path);
        nd->flags |= LOOKUP_REVAL;
        result = __link_path_walk(name, nd);
    }

    path_put(&save);

    return result;
} 


/*      
 * The bitmask for a lookup event:
 *  - follow links at the end
 *  - require a directory
 *  - ending slashes ok even for nonexistent files
 *  - internal "there are more path compnents" flag
 *  - locked when lookup done with dcache_lock held
 *  - dentry cache is untrusted; force a real lookup
 */
#define LOOKUP_FOLLOW        1                                                                              
#define LOOKUP_DIRECTORY     2
#define LOOKUP_CONTINUE      4
#define LOOKUP_PARENT       16
#define LOOKUP_REVAL        64


/* Name hashing routines. Initial hash value */
/* Hash courtesy of the R5 hash in reiserfs modulo sign bits */
#define init_name_hash()        0

/* partial hash update function. Assume roughly 4 bits per character */
unsigned long
partial_name_hash(unsigned long c, unsigned long prevhash)
{
    return (prevhash + (c << 4) + (c >> 4)) * 11;
}

/*
 * Finally: cut down the number of bits to a int value (and try to avoid
 * losing bits)
 */
unsigned long end_name_hash(unsigned long hash)
{
    return (unsigned int) hash;                                                                             
}



/*
 * Name resolution.
 * This is the basic name resolution function, turning a pathname into
 * the final dentry. We expect 'base' to be positive and a directory.
 *
 * Returns 0 and nd will have valid dentry and mnt on success.
 * Returns error and drops reference to input namei data on failure.
 */
static int __link_path_walk(const char *name, struct nameidata *nd)
{
    struct path next;
    struct inode *inode;
    int err = 0;
    unsigned int lookup_flags = nd->flags;

    while (*name=='/')
        name++;
    if (!*name)
        goto return_reval;

    inode = nd->path.dentry->d_inode;
    if (nd->depth)
        lookup_flags = LOOKUP_FOLLOW | (nd->flags & LOOKUP_CONTINUE);

    /* At this point we know we have a real path component. */
    for(;;) {
        unsigned long hash;
        struct qstr this;
        unsigned int c;

        nd->flags |= LOOKUP_CONTINUE;
        //err = exec_permission_lite(inode);
        if (err == -EAGAIN){
            //err = vfs_permission(nd, MAY_EXEC);
        }
        if (err)
            break;

        this.name = name;
        c = *(const unsigned char *)name;

        hash = init_name_hash();
        do {
            name++;
            hash = partial_name_hash(c, hash);
            c = *(const unsigned char *)name;
        } while (c && (c != '/'));
        this.len = name - (const char *) this.name;
        this.hash = end_name_hash(hash);

        /* remove trailing slashes? */               
        if (!c)
            goto last_component;
        while (*++name == '/');
        if (!*name) 
            goto last_with_slashes;
                
        /*  
         * "." and ".." are special - ".." especially so because it has
         * to be able to know about the current root directory and
         * parent relationships.
         */ 
        if (this.name[0] == '.') switch (this.len) {
            default:
                break;
            case 2:
                if (this.name[1] != '.')
                    break;
                follow_dotdot(nd);
                inode = nd->path.dentry->d_inode;
                /* fallthrough */
            case 1:
                continue;
        }
        /*  
         * See if the low-level filesystem might want
         * to use its own hash..
         */     
        if (nd->path.dentry->d_op && nd->path.dentry->d_op->d_hash) {
            err = nd->path.dentry->d_op->d_hash(nd->path.dentry,
                                &this);
            if (err < 0)
                break;
        }       
        /* This does the actual lookups.. */
        err = do_lookup(nd, &this, &next);
        if (err)
            break;
            
        err = -ENOENT;          
        inode = next.dentry->d_inode;
        if (!inode)
            goto out_dput;
        err = -ENOTDIR; 
        if (!inode->i_op)
            goto out_dput;        

        if (inode->i_op->follow_link) {
            err = do_follow_link(&next, nd);
            if (err)
                goto return_err;
            err = -ENOENT;
            inode = nd->path.dentry->d_inode;
            if (!inode)
                break;
            err = -ENOTDIR; 
            if (!inode->i_op)
                break;
        } else
            path_to_nameidata(&next, nd);
        err = -ENOTDIR; 
        if (!inode->i_op->lookup)
            break;
        continue;
        /* here ends the main loop */

last_with_slashes:
        lookup_flags |= LOOKUP_FOLLOW | LOOKUP_DIRECTORY;

last_component:                                                                                                                                                                                                                  
        /* Clear LOOKUP_CONTINUE iff it was previously unset */
        nd->flags &= lookup_flags | ~LOOKUP_CONTINUE;
        if (lookup_flags & LOOKUP_PARENT)
            goto lookup_parent;
        if (this.name[0] == '.') switch (this.len) {
            default:
                break;
            case 2: 
                if (this.name[1] != '.')
                    break;
                follow_dotdot(nd);
                inode = nd->path.dentry->d_inode;
                /* fallthrough */
            case 1:
                goto return_reval;
        }
        if (nd->path.dentry->d_op && nd->path.dentry->d_op->d_hash) {
            err = nd->path.dentry->d_op->d_hash(nd->path.dentry,
                                &this);
            if (err < 0)
                break;
        }
        err = do_lookup(nd, &this, &next);
        if (err)
            break;
        inode = next.dentry->d_inode;
        if ((lookup_flags & LOOKUP_FOLLOW)
            && inode && inode->i_op && inode->i_op->follow_link) {
            err = do_follow_link(&next, nd);
            if (err)
                goto return_err;
            inode = nd->path.dentry->d_inode;
        } else
            path_to_nameidata(&next, nd);
        err = -ENOENT;
        if (!inode)
            break;
        if (lookup_flags & LOOKUP_DIRECTORY) {
            err = -ENOTDIR; 
            if (!inode->i_op || !inode->i_op->lookup)
                break;
        }
        goto return_base;
lookup_parent:
        nd->last = this;
        nd->last_type = LAST_NORM;
        if (this.name[0] != '.')
            goto return_base;
        if (this.len == 1)
			nd->last_type = LAST_DOT;
		else if (this.len == 2 && this.name[1] == '.')
			nd->last_type = LAST_DOTDOT;
		else
			goto return_base;
return_reval:
		/*
		 * We bypassed the ordinary revalidation routines.
		 * We may need to check the cached dentry for staleness.
		 */
		if (nd->path.dentry && nd->path.dentry->d_sb &&
			(nd->path.dentry->d_sb->s_type->fs_flags & FS_REVAL_DOT)) {
			err = -ESTALE;
			/* Note: we do not d_invalidate() */
			if (!nd->path.dentry->d_op->d_revalidate(
					nd->path.dentry, nd))
				break;
		}
return_base:
		return 0;
out_dput:
		path_put_conditional(&next, nd);
		break;
	}
	path_put(&nd->path);
return_err:
	return err;
}




/*
 *  It's more convoluted than I'd like it to be, but... it's still fairly
 *  small and for now I'd prefer to have fast path as straight as possible.
 *  It _is_ time-critical.
 */
static int do_lookup(struct nameidata *nd, struct qstr *name,
             struct path *path)
{
    struct vfsmount *mnt = nd->path.mnt;
    struct dentry *dentry = __d_lookup(nd->path.dentry, name);

    if (!dentry)
        goto need_lookup;
    if (dentry->d_op && dentry->d_op->d_revalidate)
        goto need_revalidate;
done:
    path->mnt = mnt;
    path->dentry = dentry;
    __follow_mount(path);
    return 0;

need_lookup:
    dentry = real_lookup(nd->path.dentry, name, nd);
    if (IS_ERR(dentry))
        goto fail;
    goto done;

need_revalidate:
    dentry = do_revalidate(dentry, nd);
    if (!dentry)
        goto need_lookup;
    if (IS_ERR(dentry))
        goto fail;
    goto done;

fail:
    return PTR_ERR(dentry);
} 



void follow_dotdot(struct nameidata *nd)
{
    struct fs_struct *fs = current->fs;

    while(1) {
        struct vfsmount *parent;
        struct dentry *old = nd->path.dentry;

                //read_lock(&fs->lock);
        if (nd->path.dentry == fs->root.dentry &&
            nd->path.mnt == fs->root.mnt) {
                        //read_unlock(&fs->lock);
            break;
        }
                //read_unlock(&fs->lock);
        //spin_lock(&dcache_lock);
        if (nd->path.dentry != nd->path.mnt->mnt_root) {
            nd->path.dentry = dget(nd->path.dentry->d_parent);
            //spin_unlock(&dcache_lock);
            dput(old);
            break;
        }
        //spin_unlock(&dcache_lock);
        //spin_lock(&vfsmount_lock);
        parent = nd->path.mnt->mnt_parent;
        if (parent == nd->path.mnt) {
            //spin_unlock(&vfsmount_lock);
            break;
        }
        mntget(parent);
        nd->path.dentry = dget(nd->path.mnt->mnt_mountpoint);
        //spin_unlock(&vfsmount_lock);
        dput(old);
        //mntput(nd->path.mnt);
        nd->path.mnt = parent;
    }
    //follow_mount(&nd->path.mnt, &nd->path.dentry);
} 


/**     
 *  dget, dget_locked   -   get a reference to a dentry
 *  @dentry: dentry to get a reference to
 *              
 *  Given a dentry or %NULL pointer increment the reference count
 *  if appropriate and return the dentry. A dentry will not be 
 *  destroyed when it has references. dget() should never be
 *  called for dentries with zero reference counter. For these cases
 *  (preferably none, functions in dcache.c are sufficient for normal
 *  needs and they take necessary precautions) you should hold dcache_lock
 *  and call dget_locked() instead of dget().
 */     
            
static inline struct dentry *dget(struct dentry *dentry)                                                    
{           
    if (dentry) {
        BUG_ON(!atomic_read(&dentry->d_count));
        atomic_inc(&dentry->d_count);
    }   
    return dentry;
} 



/* 
 * This is dput
 *
 * This is complicated by the fact that we do not want to put
 * dentries that are no longer on any hash chain on the unused
 * list: we'd much rather just get rid of them immediately.
 *
 * However, that implies that we have to traverse the dentry
 * tree upwards to the parents which might _also_ now be
 * scheduled for deletion (it may have been only waiting for
 * its last child to go away).
 *
 * This tail recursion is done by hand as we don't want to depend
 * on the compiler to always get this right (gcc generally doesn't).
 * Real recursion would eat up our stack space.
 */

/*
 * dput - release a dentry
 * @dentry: dentry to release 
 *
 * Release a dentry. This will drop the usage count and if appropriate
 * call the dentry unlink method as well as removing it from the queues and
 * releasing its resources. If the parent dentries were scheduled for release
 * they too may now get deleted.
 *
 * no dcache lock, please.
 */

void dput(struct dentry *dentry)
{
    if (!dentry)
        return;

repeat:
    if (atomic_read(&dentry->d_count) == 1)
        might_sleep();
    if (!atomic_dec_and_lock(&dentry->d_count, &dcache_lock))
        return;

    //spin_lock(&dentry->d_lock);
    if (atomic_read(&dentry->d_count)) {
        //spin_unlock(&dentry->d_lock);
        //spin_unlock(&dcache_lock);
        return;
    }                                                        


    /*
     * AV: ->d_delete() is _NOT_ allowed to block now.
     */
    if (dentry->d_op && dentry->d_op->d_delete) {
        if (dentry->d_op->d_delete(dentry))
            goto unhash_it;
    }
    /* Unreachable? Get rid of it */
    if (d_unhashed(dentry))
        goto kill_it;
    if (list_empty(&dentry->d_lru)) {
        dentry->d_flags |= DCACHE_REFERENCED;
        dentry_lru_add(dentry);
    }
    //spin_unlock(&dentry->d_lock);
    //spin_unlock(&dcache_lock);
    return;

unhash_it:
    __d_drop(dentry);
kill_it:
    /* if dentry was on the d_lru list delete it from there */
    dentry_lru_del(dentry);
    dentry = d_kill(dentry);
    if (dentry)
        goto repeat;
}



void fput_light(struct file *file, int fput_needed)
{                                                                                                           
    if (unlikely(fput_needed))
        fput(file);
}


void fput(struct file *file)
{
    if (atomic_long_dec_and_test(&file->f_count))
        __fput(file);                                                                                       
}



/* __fput is called from task context when aio completion releases the last
 * last use of a struct file *.  Do not use otherwise.
 */ 
void __fput(struct file *file)
{
    struct dentry *dentry = file->f_path.dentry;
    struct vfsmount *mnt = file->f_path.mnt;
    struct inode *inode = dentry->d_inode;

    //might_sleep();

    //fsnotify_close(file);
    /*
     * The function eventpoll_release() should be the first called
     * in the file cleanup chain.
     */
    //eventpoll_release(file);
    //locks_remove_flock(file);

    if (file->f_op && file->f_op->release)
        file->f_op->release(inode, file);
    //security_file_free(file);
    if (unlikely(S_ISCHR(inode->i_mode) && inode->i_cdev != NULL))
        cdev_put(inode->i_cdev);
    fops_put(file->f_op);
    put_pid(file->f_owner.pid);
    file_kill(file);
    if (file->f_mode & FMODE_WRITE)
        drop_file_write_access(file);
    file->f_path.dentry = NULL;
    file->f_path.mnt = NULL;
    file_free(file);
    dput(dentry);
    mntput(mnt);
} 



/**
 * file_permission  -  check for additional access rights to a given file
 * @file:   file to check access rights for
 * @mask:   right to check for (%MAY_READ, %MAY_WRITE, %MAY_EXEC)
 *
 * Used to check for read/write/execute permissions on an already opened
 * file.
 *
 * Note:
 *  Do not use this function in new code.  All access checks should
 *  be done using vfs_permission().
 */
int file_permission(struct file *file, int mask)                                                            
{
	//TODO
    return 0;
}



/**
 * path_get - get a reference to a path
 * @path: path to get the reference to
 *
 * Given a path increment the reference count to the dentry and the vfsmount.
 */
void path_get(struct path *path)
{                                                                                                           
    mntget(path->mnt);
    dget(path->dentry);
}



struct vfsmount *mntget(struct vfsmount *mnt)                                                 
{   
    if (mnt)
        atomic_inc(&mnt->mnt_count);
    return mnt;
}






#define __NFDBITS   (8 * sizeof(unsigned long))


void __FD_SET(unsigned long fd, __kernel_fd_set *fdsetp)
{
    unsigned long _tmp = fd / __NFDBITS;
    unsigned long _rem = fd % __NFDBITS;
    fdsetp->fds_bits[_tmp] |= (1UL<<_rem);
}

void __FD_CLR(unsigned long fd, __kernel_fd_set *fdsetp)
{
    unsigned long _tmp = fd / __NFDBITS;
    unsigned long _rem = fd % __NFDBITS;
    fdsetp->fds_bits[_tmp] &= ~(1UL<<_rem);
}




#define FD_SET(fd,fdsetp)   __FD_SET(fd,fdsetp)
#define FD_CLR(fd,fdsetp)   __FD_CLR(fd,fdsetp)




/*
 * allocate a file descriptor, mark it busy.
 */
int alloc_fd(unsigned start, unsigned flags)
{
    struct files_struct *files = current->files;
    unsigned int fd;
    int error;
    struct fdtable *fdt;

    //spin_lock(&files->file_lock);
repeat:
    fdt = files->fdt;
    //下面是fd获取一个合适的fd
    fd = start;
    if (fd < files->next_fd)
        fd = files->next_fd;

    if (fd < fdt->max_fds)
        fd = find_next_zero_bit(fdt->open_fds->fds_bits,
                       fdt->max_fds, fd);

    error = expand_files(files, fd);//如果files中的fdtables满了，则进行扩展
    if (error < 0)
        goto out;

    /*
     * If we needed to expand the fs array we
     * might have blocked - try again.
     */
    if (error)
        goto repeat;

    if (start <= files->next_fd)
        files->next_fd = fd + 1;

    FD_SET(fd, fdt->open_fds);
    if (flags & O_CLOEXEC)
        FD_SET(fd, fdt->close_on_exec);
    else
        FD_CLR(fd, fdt->close_on_exec);
    error = fd;
#if 1
    /* Sanity check */
    if (fdt->fd[fd]) != NULL) {
        fdt->fd[fd] = NULL;
    }
#endif

out:
    //spin_unlock(&files->file_lock);                
    return error;
}



/*                                                                                                          
 * Expand files.
 * This function will expand the file structures, if the requested size exceeds
 * the current capacity and there is room for expansion.
 * Return <0 error code on error; 0 when nothing done; 1 when files were
 * expanded and execution may have blocked.
 * The files->file_lock should be held on entry, and will be held on exit.
 */
int expand_files(struct files_struct *files, int nr)
{
    struct fdtable *fdt;

    fdt = files->fdt;

    /*
     * N.B. For clone tasks sharing a files structure, this test
     * will limit the total number of files that can be opened.
     */
    //if (nr >= current->signal->rlim[RLIMIT_NOFILE].rlim_cur)
    //    return -EMFILE;

    /* Do we need to expand? */
    if (nr < fdt->max_fds)
        return 0;

    /* Can we expand? */
    //if (nr >= sysctl_nr_open)
    //    return -EMFILE;

    /* All good, so we try */
    return expand_fdtable(files, nr);
}


/*
 * Expand the file descriptor table.
 * This function will allocate a new fdtable and both fd array and fdset, of
 * the given size.
 * Return <0 error code on error; 1 on successful completion.
 * The files->file_lock should be held on entry, and will be held on exit.
 */
static int expand_fdtable(struct files_struct *files, int nr)
{
	//TODO

	return 1;
}



/**
 * __ffs - find first bit in word.
 * @word: The word to search
 *
 * Undefined if no bit exists, so code should check against 0 first.
 */
unsigned long __ffs(unsigned long word)
{
    int num = 0;

#if BITS_PER_LONG == 64
    if ((word & 0xffffffff) == 0) {
        num += 32;
        word >>= 32;
    }
#endif
    if ((word & 0xffff) == 0) {                         // 低16位没有1值,那么num加16,同时高16位移动到低16位
        num += 16;                                      // 这样低16位永远都是去掉根本不存在的那种必然情况后的数据.
        word >>= 16;
    }
    if ((word & 0xff) == 0) {                           // 低8位是否有
        num += 8;
        word >>= 8;
    }
    if ((word & 0xf) == 0) {
        num += 4;
        word >>= 4;
    }
    if ((word & 0x3) == 0) {
        num += 2;
        word >>= 2;
    }
    if ((word & 0x1) == 0)
        num += 1;
    return num;                                         // 这样num就是出现1的偏移值了.[luther.gliethttp]
}



#define BITOP_WORD(nr)      ((nr) / BITS_PER_LONG)
#define ffz(x)  __ffs(~(x))                             // __ffs找到第一次出现1值的偏移值,从bit0开始到bit31依次找[luther.gliethttp]

/*查找第一个不为0的位
 * This implementation of find_{first,next}_zero_bit was stolen from
 * Linus' asm-alpha/bitops.h.
 */
unsigned long find_next_zero_bit(const unsigned long *addr, unsigned long size,
                 unsigned long offset)
{
    const unsigned long *p = addr + BITOP_WORD(offset);		// offset位于p指向的long地址32位空间
    unsigned long result = offset & ~(BITS_PER_LONG-1);		// offset是第result个4字节
    unsigned long tmp;

    if (offset >= size)
        return size;
    size -= result;									// 调整32位整倍数上
    offset %= BITS_PER_LONG;						// offset位于32位的第几位
    if (offset) {									// offset不在一个long数据的第0位上,在1-31位中[luther.gliethttp]
        tmp = *(p++);
        tmp |= ~0UL >> (BITS_PER_LONG - offset);	// 将0-offset数据填充上1.
        if (size < BITS_PER_LONG)					// 不足32bits
            goto found_first;
        if (~tmp)									// 取非非0说明含有0值
            goto found_middle;
        size -= BITS_PER_LONG;						// 如果上面~tmp等于0,那么说明该*p数据为32位全1.[luther.gliethttp]
        result += BITS_PER_LONG;
    }
    while (size & ~(BITS_PER_LONG-1)) {				// 好了,执行到这里,我们的offset已经处在4字节的第0位上,下面进行
        if (~(tmp = *(p++)))						// 4字节快速查询.如果~tmp非0,说明该32位数据中含有0数据,找到.[luther.gliethttp]
            goto found_middle;
        result += BITS_PER_LONG;					// 到下一个4字节空间
        size -= BITS_PER_LONG;						// 减少4字节数据
    }
    if (!size)										// size等于0,说明首先size等于4字节整倍数,其次所有数据已经查完,
        return result;								// 所有数据全部为1,没有发现0位,result等于size.[luther.gliethttp]
    tmp = *p;										// size不是32位整倍数,还剩几个bit没有检查,继续执行下面检查工作.[luther.gliethtp]

found_first:
    tmp |= ~0UL << size;							// 现在0-size为有效数据,size-31为未使用空间,所以先将size-31置成全1.
    if (tmp == ~0UL)    /* Are any bits zero? */		// 如果tmp全1,那么说明就没找找1个
        return result + size;   /* Nope. */			// result+size就等于函数传入的参数size大小.[luther.gliethttp]
found_middle:
    return result + ffz(tmp);						// 我们在32位数据的0-31中发现必定存在0位值,计算他是第几位.[luther.gliethttp]
}




/*
 * Careful here! We test whether the file pointer is NULL before                                            
 * releasing the fd. This ensures that one clone task can't release
 * an fd while another clone is opening it.
 */
long sys_close(unsigned int fd)
{
    struct file * filp;
    struct files_struct *files = current->files;
    struct fdtable *fdt;
    int retval;

    spin_lock(&files->file_lock);
    fdt = files_fdtable(files);
    if (fd >= fdt->max_fds)
        goto out_unlock;
    filp = fdt->fd[fd];
    if (!filp)  
        goto out_unlock;
    rcu_assign_pointer(fdt->fd[fd], NULL);
    FD_CLR(fd, fdt->close_on_exec);
    __put_unused_fd(files, fd);
    spin_unlock(&files->file_lock);
    retval = filp_close(filp, files);
        
    /* can't restart close syscall because file table entry was cleared */
    if (unlikely(retval == -ERESTARTSYS ||
             retval == -ERESTARTNOINTR ||
             retval == -ERESTARTNOHAND ||
             retval == -ERESTART_RESTARTBLOCK))
        retval = -EINTR;

    return retval;
    
out_unlock:
    spin_unlock(&files->file_lock);
    return -EBADF;
} 



ssize_t sys_read(unsigned int fd, char * buf, size_t count)                               
{
	int ret;

	ret = ext2_read_data(file->volume, file->inode, file->offset,
			     buf, count);
	if (ret == -1)
		return -1;
	file->offset += ret;
	return ret;

}


ssize_t sys_write(unsigned int fd, const char * buf, size_t count)
{
    struct file *file;
    ssize_t ret = -EBADF;
    int fput_needed;

    file = fget_light(fd, &fput_needed);
    if (file) {
        loff_t pos = file_pos_read(file);
        ret = vfs_write(file, buf, count, &pos);
        file_pos_write(file, pos);
        fput_light(file, fput_needed);
    }

    return ret;
}


/*
 * Lightweight file lookup - no refcnt increment if fd table isn't shared. 
 * You can use this only if it is guranteed that the current task already 
 * holds a refcnt to that file. That check has to be done at fget() only
 * and a flag is returned to be passed to the corresponding fput_light().
 * There must not be a cloning between an fget_light/fput_light pair.
 */
struct file *fget_light(unsigned int fd, int *fput_needed)
{
    struct file *file;
    struct files_struct *files = current->files;

    *fput_needed = 0;
    if (likely((atomic_read(&files->count) == 1))) {
        file = fcheck_files(files, fd);
    } else {
        rcu_read_lock();
        file = fcheck_files(files, fd);
        if (file) {
            if (atomic_long_inc_not_zero(&file->f_count))
                *fput_needed = 1;
            else
                /* Didn't get the reference, someone's freed */
                file = NULL;
        }
        rcu_read_unlock();
    }

    return file;
} 


struct file * fcheck_files(struct files_struct *files, unsigned int fd)
{
    struct file * file = NULL;
    struct fdtable *fdt = files->fdt;

    if (fd < fdt->max_fds)
        file = fdt->fd[fd];
    return file;
}  

