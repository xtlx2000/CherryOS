#ifndef _TERMINAL_H_
#define _TERMINAL_H_

struct coverage_struct;

struct terminal {
	struct coverage_struct *sht;
	int cur_x, 
		cur_y, 
		cur_c;
	struct timer_struct *timer;
};

void terminal_task(struct coverage_struct *cover, int memtotal);
void terminal_putchar(struct terminal *cons, int chr, char move);
void terminal_newline(struct terminal *cons);
void terminal_putstr0(struct terminal *cons, char *s);
void terminal_putstr1(struct terminal *cons, char *s, int l);
void terminal_runcmd(char *cmdline, struct terminal *cons, int *fat, int memtotal);
void cmd_mem(struct terminal *cons, int memtotal);
void cmd_cls(struct terminal *cons);
void cmd_dir(struct terminal *cons);
void cmd_type(struct terminal *cons, int *fat, char *cmdline);
int cmd_app(struct terminal *cons, int *fat, char *cmdline);
int *hrb_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx, int eax);
int *inthandler0d(int *esp);
int *inthandler0c(int *esp);
void hrb_api_linewin(struct coverage_struct *sht, int x0, int y0, int x1, int y1, int col);

#endif
