#include "bitmap.h"

#include <stdio.h>
#include <stdlib.h>


void bitmap_construct(struct Bitmap *bitmap, int bits)
{
	bitmap->m_iBits = bits;
	bitmap->m_pMap = malloc(sizeof(int)*(1+bits/WORD))
}

void bitmap_destroy(struct Bitmap *bitmap)
{
	if(bitmap->m_pMap){
		free(bitmap->m_pMap);
	}
	bitmap->m_pMap = NULL;
}

void bitmap_setall(struct Bitmap *bitmap)
{
	int i;
	for(i = 0; i < bitmap->m_iBits; ++i){
		bitmap_set(bitmap, i);
	}
}

void bitmap_clearall(struct Bitmap *bitmap)
{
	int i;
	for(i = 0; i < bitmap->m_iBits; ++i){
		bitmap_clear(bitmap, i);
	}
}

void bitmap_set(struct Bitmap *bitmap, int i)
{
	bitmap->m_pMap[i >> SHIFT] |= (1 << (i & MASK));
}

void bitmap_clear(struct Bitmap *bitmap, int i)
{
	bitmap->m_pMap[i >> SHIFT] &= ~(1 << (i & MASK));
}

int bitmap_test(struct Bitmap *bitmap, int i)
{
	if(bitmap->m_pMap[i >> SHIFT] & (1 << (i & MASK))){
		return 1;
	}else{
		return 0;
	}
}

int bitmap_getAFreeBit(struct Bitmap *bitmap)
{
	for(int i = 1; i < bitmap->m_iBits; ++i){
		if(bitmap_test(bitmap, i) == 0){
			bitmap_set(bitmap, i);
			return i;
		}
	}
}

