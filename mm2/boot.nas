; cherryOS boot asm
; TAB=4

[INSTRSET "i486p"]

VBEMODE	EQU		0x105			; 默认使用1024 x  768 x 8bit彩色的显示模式
; 这是其他显示模式的值
;	0x100 :  640 x  400 x 8bit彩色
;	0x101 :  640 x  480 x 8bit彩色
;	0x103 :  800 x  600 x 8bit彩色
;	0x105 : 1024 x  768 x 8bit彩色
;	0x107 : 1280 x 1024 x 8bit彩色

BOTPAK	EQU		0x00280000		; 加载kernel
DSKCAC	EQU		0x00100000		; 磁盘缓存的位置 
DSKCAC0	EQU		0x00008000		; 磁盘缓存（实模式）的位置

; BOOT_INFO相关
CYLS	EQU		0x0ff0			; 设定启动区  
LEDS	EQU		0x0ff1			;键盘LED指示灯状态  
VMODE	EQU		0x0ff2			; 颜色的位数 
SCRNX	EQU		0x0ff4			;分辨率的X  
SCRNY	EQU		0x0ff6			;分辨率的Y
VRAM	EQU		0x0ff8			;图像缓冲区的开始地址 

		ORG		0xc200			;程序装载在内存中位置

; 确认VBE是否存在

		MOV		AX,0x9000
		MOV		ES,AX
		MOV		DI,0
		MOV		AX,0x4f00
		INT		0x10
		CMP		AX,0x004f
		JNE		scrn320

; 检查VBE的版本

		MOV		AX,[ES:DI+4]
		CMP		AX,0x0200
		JB		scrn320			; if (AX < 0x0200) goto scrn320

; 取得画面模式信息

		MOV		CX,VBEMODE
		MOV		AX,0x4f01
		INT		0x10
		CMP		AX,0x004f
		JNE		scrn320

; 画面模式信息的确认

		CMP		BYTE [ES:DI+0x19],8
		JNE		scrn320
		CMP		BYTE [ES:DI+0x1b],4
		JNE		scrn320
		MOV		AX,[ES:DI+0x00]
		AND		AX,0x0080
		JZ		scrn320			; 模式属性的bit7是0，所以放弃

; 画面模式的切换

		MOV		BX,VBEMODE+0x4000
		MOV		AX,0x4f02
		INT		0x10
		MOV		BYTE [VMODE],8	; 记下画面模式
		MOV		AX,[ES:DI+0x12]
		MOV		[SCRNX],AX
		MOV		AX,[ES:DI+0x14]
		MOV		[SCRNY],AX
		MOV		EAX,[ES:DI+0x28]
		MOV		[VRAM],EAX
		JMP		keystatus

scrn320:
		MOV		AL,0x13			; VGA图，320*200*8bit彩色
		MOV		AH,0x00
		INT		0x10
		MOV		BYTE [VMODE],8	;; 记下画面模式
		MOV		WORD [SCRNX],320
		MOV		WORD [SCRNY],200
		MOV		DWORD [VRAM],0x000a0000

;从BIOS中获取LED灯的状态记录在LEDS变量中

keystatus:
		MOV		AH,0x02
		INT		0x16 			; keyboard BIOS
		MOV		[LEDS],AL

; PIC关闭一切中断
;	根据AT兼容机的规格，如果要初始化PIC，
;	必须在CTL之前进行，否则有时会挂起。
;	随后进行PIC的初始化

		MOV		AL,0xff		;0xff是全部中断标记
		OUT		0x21,AL		;禁止主PIC全部中断，等同于io_out(PIC0_IMR, 0xff)
		NOP				; 如果连续执行OUT指令，有些时候无法正常运行，因此CPU休息1个时钟
		OUT		0xa1,AL		;禁止从PIC全部中断，等同于io_out(PIC1_IMR, 0xff)

		CLI				; 禁止CPU级别的中断

;为了让CPU能够访问1MB以上的内存空间，设定A20GATE

		CALL	waitkbdout		;这里的waitkbdout等同于wait_KBC_sendready()
		MOV		AL,0xd1		;#define KEYCMD_WRITE_OUTPORT  0xd1
		OUT		0x64,AL		;io_out8(PORT_KEYCMD,  KEYCMD_WRITE_OUTPORT)
		CALL	waitkbdout		;wait_KBC_sendready()
		MOV		AL,0xdf		; enable A20  ;#define KBC_OUTPORT_A20G_ENABLE  0xdf
		OUT		0x60,AL		;io_out8(PORT_KEYDAT,  KBC_OUTPORT_A20G_ENABLE)
		CALL	waitkbdout		;wait_KBC_sendready()  为了等待完成指令

; 切换到保护模式s

		LGDT	[GDTR0]			; 设定临时GDT
		MOV		EAX,CR0
		AND		EAX,0x7fffffff	; 设置bit31为0(为了颁)，最高位置0
		OR		EAX,0x00000001	; 设置bit0为1(为了切换到保护模式)，最低位置1
		MOV		CR0,EAX		;置位后的值返给CR0，这样就完成了模式转换，进入到不用颁的保护模式。
		JMP		pipelineflush
pipelineflush:
		MOV		AX,1*8		;  可读写的段32bit
		MOV		DS,AX
		MOV		ES,AX
		MOV		FS,AX
		MOV		GS,AX
		MOV		SS,AX

; kernel的转送

		MOV		ESI,kernel	; 转送源
		MOV		EDI,BOTPAK		; 转送目的地
		MOV		ECX,512*1024/4
		CALL	memcpy

; 磁盘数据最终转送到它本来的位置去

; 首先从启动扇区开始

		MOV		ESI,0x7c00		; 转送源
		MOV		EDI,DSKCAC		; 转送目的地
		MOV		ECX,512/4
		CALL	memcpy

; 所有剩下的

		MOV		ESI,DSKCAC0+512		; 转送源
		MOV		EDI,DSKCAC+512		;  转送目的地
		MOV		ECX,0
		MOV		CL,BYTE [CYLS]
		IMUL	ECX,512*18*2/4			; 从柱面变化成字节数/4
		SUB		ECX,512/4		; 减去IPL
		CALL	memcpy

; 必须由asmhead完成的工作，至此全部完毕
; 以后就交给kernel来完成

; kernel的启动

		MOV		EBX,BOTPAK
		MOV		ECX,[EBX+16]
		ADD		ECX,3			; ECX += 3;
		SHR		ECX,2			; ECX /= 4;
		JZ		skip			; 没有要拷贝的东西时候
		MOV		ESI,[EBX+20]		; 拷贝源
		ADD		ESI,EBX
		MOV		EDI,[EBX+12]		; 拷贝目的
		CALL	memcpy
skip:
		MOV		ESP,[EBX+12]		; 栈初始值
		JMP		DWORD 2*8:0x0000001b

waitkbdout:
		IN		 AL,0x64
		AND		 AL,0x02
		IN		 AL,0x60 		; 偐傜撉傒(庴怣僶僢僼傽偑埆偝傪偟側偄傛偆偵)
		JNZ		waitkbdout		; AND的结果如果不是0，就跳到waitkbdout
		RET

memcpy:
		MOV		EAX,[ESI]
		ADD		ESI,4
		MOV		[EDI],EAX
		ADD		EDI,4
		SUB		ECX,1
		JNZ		memcpy			; 减法运算的结果如果不是0，就跳转到memcpy
		RET
; memcpy偼傾僪儗僗僒僀僘僾儕僼傿僋僗傪擖傟朰傟側偗傟偽丄僗僩儕儞僌柦椷偱傕彂偗傞

		ALIGNB	16
GDT0:
		RESB	8				; NULL selector
		DW		0xffff,0x0000,0x9200,0x00cf	; 可以读写的段(segment)32bit
		DW		0xffff,0x0000,0x9a28,0x0047	; 可以执行的段(segment)32bit (kernel用)

		DW		0
GDTR0:
		DW		8*3-1
		DD		GDT0

		ALIGNB	16
kernel:
