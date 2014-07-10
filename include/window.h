#ifndef _WINDOW_H_
#define _WINDOW_H_

struct coverage_struct;

void make_window8(unsigned char *buf, int xsize, int ysize, char *title, char act);
void putfonts8_asc_sht(struct coverage_struct *sht, int x, int y, int c, int b, char *s, int l);
void make_textbox8(struct coverage_struct *sht, int x0, int y0, int sx, int sy, int c);
void make_wtitle8(unsigned char *buf, int xsize, char *title, char act);
void change_wtitle8(struct coverage_struct *sht, char act);


#endif
