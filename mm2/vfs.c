#include "vfs.h"
#include "list.h"


int vfs_init()
{
	// 1  init file_system_type lists
	file_system_type_root.name = "filesystem";
	file_system_type_root.fs_flags = 0;
	file_system_type_root.next = NULL;
	INIT_LIST_HEAD(&file_system_type_root.fs_supers);

	// 2 init vfsmount list
	
	return 0;
}

