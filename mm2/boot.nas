; cherryOS boot asm
; TAB=4

[INSTRSET "i486p"]

VBEMODE	EQU		0x105			; Ĭ��ʹ��1024 x  768 x 8bit��ɫ����ʾģʽ
; ����������ʾģʽ��ֵ
;	0x100 :  640 x  400 x 8bit��ɫ
;	0x101 :  640 x  480 x 8bit��ɫ
;	0x103 :  800 x  600 x 8bit��ɫ
;	0x105 : 1024 x  768 x 8bit��ɫ
;	0x107 : 1280 x 1024 x 8bit��ɫ

BOTPAK	EQU		0x00280000		; ����kernel
DSKCAC	EQU		0x00100000		; ���̻����λ�� 
DSKCAC0	EQU		0x00008000		; ���̻��棨ʵģʽ����λ��

; BOOT_INFO���
CYLS	EQU		0x0ff0			; �趨������  
LEDS	EQU		0x0ff1			;����LEDָʾ��״̬  
VMODE	EQU		0x0ff2			; ��ɫ��λ�� 
SCRNX	EQU		0x0ff4			;�ֱ��ʵ�X  
SCRNY	EQU		0x0ff6			;�ֱ��ʵ�Y
VRAM	EQU		0x0ff8			;ͼ�񻺳����Ŀ�ʼ��ַ 

		ORG		0xc200			;����װ�����ڴ���λ��

; ȷ��VBE�Ƿ����

		MOV		AX,0x9000
		MOV		ES,AX
		MOV		DI,0
		MOV		AX,0x4f00
		INT		0x10
		CMP		AX,0x004f
		JNE		scrn320

; ���VBE�İ汾

		MOV		AX,[ES:DI+4]
		CMP		AX,0x0200
		JB		scrn320			; if (AX < 0x0200) goto scrn320

; ȡ�û���ģʽ��Ϣ

		MOV		CX,VBEMODE
		MOV		AX,0x4f01
		INT		0x10
		CMP		AX,0x004f
		JNE		scrn320

; ����ģʽ��Ϣ��ȷ��

		CMP		BYTE [ES:DI+0x19],8
		JNE		scrn320
		CMP		BYTE [ES:DI+0x1b],4
		JNE		scrn320
		MOV		AX,[ES:DI+0x00]
		AND		AX,0x0080
		JZ		scrn320			; ģʽ���Ե�bit7��0�����Է���

; ����ģʽ���л�

		MOV		BX,VBEMODE+0x4000
		MOV		AX,0x4f02
		INT		0x10
		MOV		BYTE [VMODE],8	; ���»���ģʽ
		MOV		AX,[ES:DI+0x12]
		MOV		[SCRNX],AX
		MOV		AX,[ES:DI+0x14]
		MOV		[SCRNY],AX
		MOV		EAX,[ES:DI+0x28]
		MOV		[VRAM],EAX
		JMP		keystatus

scrn320:
		MOV		AL,0x13			; VGAͼ��320*200*8bit��ɫ
		MOV		AH,0x00
		INT		0x10
		MOV		BYTE [VMODE],8	;; ���»���ģʽ
		MOV		WORD [SCRNX],320
		MOV		WORD [SCRNY],200
		MOV		DWORD [VRAM],0x000a0000

;��BIOS�л�ȡLED�Ƶ�״̬��¼��LEDS������

keystatus:
		MOV		AH,0x02
		INT		0x16 			; keyboard BIOS
		MOV		[LEDS],AL

; PIC�ر�һ���ж�
;	����AT���ݻ��Ĺ�����Ҫ��ʼ��PIC��
;	������CTL֮ǰ���У�������ʱ�����
;	������PIC�ĳ�ʼ��

		MOV		AL,0xff		;0xff��ȫ���жϱ��
		OUT		0x21,AL		;��ֹ��PICȫ���жϣ���ͬ��io_out(PIC0_IMR, 0xff)
		NOP				; �������ִ��OUTָ���Щʱ���޷��������У����CPU��Ϣ1��ʱ��
		OUT		0xa1,AL		;��ֹ��PICȫ���жϣ���ͬ��io_out(PIC1_IMR, 0xff)

		CLI				; ��ֹCPU������ж�

;Ϊ����CPU�ܹ�����1MB���ϵ��ڴ�ռ䣬�趨A20GATE

		CALL	waitkbdout		;�����waitkbdout��ͬ��wait_KBC_sendready()
		MOV		AL,0xd1		;#define KEYCMD_WRITE_OUTPORT  0xd1
		OUT		0x64,AL		;io_out8(PORT_KEYCMD,  KEYCMD_WRITE_OUTPORT)
		CALL	waitkbdout		;wait_KBC_sendready()
		MOV		AL,0xdf		; enable A20  ;#define KBC_OUTPORT_A20G_ENABLE  0xdf
		OUT		0x60,AL		;io_out8(PORT_KEYDAT,  KBC_OUTPORT_A20G_ENABLE)
		CALL	waitkbdout		;wait_KBC_sendready()  Ϊ�˵ȴ����ָ��

; �л�������ģʽs

		LGDT	[GDTR0]			; �趨��ʱGDT
		MOV		EAX,CR0
		AND		EAX,0x7fffffff	; ����bit31Ϊ0(Ϊ�˰�)�����λ��0
		OR		EAX,0x00000001	; ����bit0Ϊ1(Ϊ���л�������ģʽ)�����λ��1
		MOV		CR0,EAX		;��λ���ֵ����CR0�������������ģʽת�������뵽���ð�ı���ģʽ��
		JMP		pipelineflush
pipelineflush:
		MOV		AX,1*8		;  �ɶ�д�Ķ�32bit
		MOV		DS,AX
		MOV		ES,AX
		MOV		FS,AX
		MOV		GS,AX
		MOV		SS,AX

; kernel��ת��

		MOV		ESI,kernel	; ת��Դ
		MOV		EDI,BOTPAK		; ת��Ŀ�ĵ�
		MOV		ECX,512*1024/4
		CALL	memcpy

; ������������ת�͵���������λ��ȥ

; ���ȴ�����������ʼ

		MOV		ESI,0x7c00		; ת��Դ
		MOV		EDI,DSKCAC		; ת��Ŀ�ĵ�
		MOV		ECX,512/4
		CALL	memcpy

; ����ʣ�µ�

		MOV		ESI,DSKCAC0+512		; ת��Դ
		MOV		EDI,DSKCAC+512		;  ת��Ŀ�ĵ�
		MOV		ECX,0
		MOV		CL,BYTE [CYLS]
		IMUL	ECX,512*18*2/4			; ������仯���ֽ���/4
		SUB		ECX,512/4		; ��ȥIPL
		CALL	memcpy

; ������asmhead��ɵĹ���������ȫ�����
; �Ժ�ͽ���kernel�����

; kernel������

		MOV		EBX,BOTPAK
		MOV		ECX,[EBX+16]
		ADD		ECX,3			; ECX += 3;
		SHR		ECX,2			; ECX /= 4;
		JZ		skip			; û��Ҫ�����Ķ���ʱ��
		MOV		ESI,[EBX+20]		; ����Դ
		ADD		ESI,EBX
		MOV		EDI,[EBX+12]		; ����Ŀ��
		CALL	memcpy
skip:
		MOV		ESP,[EBX+12]		; ջ��ʼֵ
		JMP		DWORD 2*8:0x0000001b

waitkbdout:
		IN		 AL,0x64
		AND		 AL,0x02
		IN		 AL,0x60 		; ����ǂ�(��M�o�b�t�@�����������Ȃ��悤��)
		JNZ		waitkbdout		; AND�Ľ���������0��������waitkbdout
		RET

memcpy:
		MOV		EAX,[ESI]
		ADD		ESI,4
		MOV		[EDI],EAX
		ADD		EDI,4
		SUB		ECX,1
		JNZ		memcpy			; ��������Ľ���������0������ת��memcpy
		RET
; memcpy�̓A�h���X�T�C�Y�v���t�B�N�X�����Y��Ȃ���΁A�X�g�����O���߂ł�������

		ALIGNB	16
GDT0:
		RESB	8				; NULL selector
		DW		0xffff,0x0000,0x9200,0x00cf	; ���Զ�д�Ķ�(segment)32bit
		DW		0xffff,0x0000,0x9a28,0x0047	; ����ִ�еĶ�(segment)32bit (kernel��)

		DW		0
GDTR0:
		DW		8*3-1
		DD		GDT0

		ALIGNB	16
kernel:
