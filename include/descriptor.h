#ifndef _DESCRIPTOR_H_
#define _DESCRIPTOR_H_


struct segment_descriptor {
	short 	limit_low, 
			base_low;
	char 	base_mid, 
			access_right;
	char 	limit_high, 
			base_high;
};

struct gate_descriptor {
	short 	offset_low, 
			selector;
	char 	dw_count, 
			access_right;
	short 	offset_high;
};

void init_gdtidt(void);
void set_segmdesc(struct segment_descriptor *sd, unsigned int limit, 
						int base, int ar);
void set_gatedesc(struct gate_descriptor *gd, int offset, int selector, int ar);


#define ADR_IDT			0x0026f800
#define LIMIT_IDT		0x000007ff
#define ADR_GDT			0x00270000
#define LIMIT_GDT		0x0000ffff
#define ADR_BOTPAK		0x00280000
#define LIMIT_BOTPAK	0x0007ffff
#define AR_DATA32_RW	0x4092
#define AR_CODE32_ER	0x409a
#define AR_TSS32		0x0089
#define AR_INTGATE32	0x008e



#endif