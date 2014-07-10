#include "vfs.h"
#include "list.h"


int vfs_init()
{
	// 1  init file_system_type lists
	/*
	file_system_type_root.name = "filesystem";
	file_system_type_root.fs_flags = 0;
	file_system_type_root.next = NULL;
	INIT_LIST_HEAD(&file_system_type_root.fs_supers);
	*/

	file_systems = NULL;

	// 2 init vfsmount list
	/*
	INIT_LIST_HEAD(&vfsmntlist.mnt_hash);
	INIT_LIST_HEAD(&vfsmntlist.mnt_mounts);
	INIT_LIST_HEAD(&vfsmntlist.mnt_child);
	INIT_LIST_HEAD(&vfsmntlist.mnt_list);
	INIT_LIST_HEAD(&vfsmntlist.mnt_expire);
	INIT_LIST_HEAD(&vfsmntlist.mnt_share);
	INIT_LIST_HEAD(&vfsmntlist.mnt_slave_list);
	INIT_LIST_HEAD(&vfsmntlist.mnt_slave);
	*/
	vfsmntlist = NULL;
	
	return 0;
}

