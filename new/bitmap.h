#ifndef _BITMAP_H_
#define _BITMAP_H_

#define BITMAP_WORD 32  
#define BITMAP_SHIFT 5 ////移动5个位,左移则相当于乘以32,右移相当于除以32取整  
#define BITMAP_MASK 0x1F //16进制下的31  

struct Bitmap
{
	int *m_pMap;
	int m_iBits;
};

void bitmap_construct(struct Bitmap *bitmap, int bits);

void bitmap_destroy(struct Bitmap *bitmap);

void bitmap_setall(struct Bitmap *bitmap);

void bitmap_clearall(struct Bitmap *bitmap);

void bitmap_set(struct Bitmap *bitmap, int i);

void bitmap_clear(struct Bitmap *bitmap, int i);

int bitmap_test(struct Bitmap *bitmap, int i);

int bitmap_getAFreeBit(struct Bitmap *bitmap);

#endif
