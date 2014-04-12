#ifndef _FS_H_
#define _FS_H_


/* open/fcntl - O_SYNC is only implemented on blocks devices and on files
   located on an ext2 file system */
#define O_ACCMODE   00000003
#define O_CREAT      01000  /* not fcntl */
#define O_TRUNC      02000  /* not fcntl */
#define O_EXCL       04000  /* not fcntl */
#define O_NOCTTY    010000  /* not fcntl */

#define O_NONBLOCK   00004
#define O_APPEND     00010
#define O_SYNC      040000
#define O_DIRECTORY 0100000 /* must be a directory */
#define O_NOFOLLOW  0200000 /* don't follow links */
#define O_LARGEFILE 0400000 /* will be set by the kernel on every open */
#define O_DIRECT    02000000 /* direct disk access - should check with OSF/1 */
#define O_NOATIME   04000000
#define O_CLOEXEC   010000000 /* set close_on_exec */   




int fs_init();

long sys_mount(char *dev_name, char *dir_name, char *type, unsigned long flags, void *data);
long sys_open(const char *filename, int flags, int mode);
long sys_close(unsigned int fd);
ssize_t sys_read(unsigned int fd, char *buf, size_t count);
ssize_t sys_write(unsigned int fd, const char *buf, size_t count);




#endif
