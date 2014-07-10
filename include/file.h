#ifndef _FILE_H_
#define _FILE_H_

struct file_struct {
	unsigned char 	name[8], 
					ext[3], 
					type;
	char 			reserve[10];
	unsigned short 	time, 
					date, 
					clustno;
	unsigned int 	size;
};

void file_readfat(int *fat, unsigned char *img);
void file_loadfile(int clustno, int size, char *buf, int *fat, char *img);
struct file_struct *file_search(char *name, struct file_struct *finfo, int max);


#endif
