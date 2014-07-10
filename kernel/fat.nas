; cherryOS-ipl
; TAB=4

CYLS	EQU		10				; cylinder num

		ORG		0x7c00			; load mem address

; FAT12 format

		JMP		entry
		DB		0x90
		DB		"cherryOS"		;name
		DW		512				;sector size
		DB		1				;cluster
		DW		1				;FAT start
		DB		2				;FAT num
		DW		224				;root size
		DW		2880			;disk size
		DB		0xf0				;disk kind
		DW		9				;FAT length
		DW		18				;sector num
		DW		2				;head num
		DD		0				
		DD		2880			
		DB		0,0,0x29		
		DD		0xffffffff		
		DB		"cherryOSIMG"	; disk name
		DB		"FAT12   "		; disk format
		RESB	18				; non

; core

entry:
		MOV		AX,0				;register init
		MOV		SS,AX			;SS
		MOV		SP,0x7c00		;SP
		MOV		DS,AX			;DS

; read disk

		MOV		AX,0x0820		;load mem
		MOV		ES,AX
		MOV		CH,0			
		MOV		DH,0			
		MOV		CL,2				
readloop:
		MOV		SI,0				; SI
retry:
		MOV		AH,0x02			; AH=0x02 : read disk
		MOV		AL,1				
		MOV		BX,0
		MOV		DL,0x00			
		INT		0x13			;BIOS API
		JNC		next				
		ADD		SI,1				
		CMP		SI,5				
		JAE		error			
		MOV		AH,0x00
		MOV		DL,0x00			
		INT		0x13			
		JMP		retry
next:
		MOV		AX,ES			;0x200  
		ADD		AX,0x0020
		MOV		ES,AX			
		ADD		CL,1				
		CMP		CL,18			
		JBE		readloop			
		MOV		CL,1
		ADD		DH,1
		CMP		DH,2
		JB		readloop		
		MOV		DH,0
		ADD		CH,1
		CMP		CH,CYLS
		JB		readloop		



		MOV		[0x0ff0],CH		;IPL
		JMP		0xc200

error:
		MOV		AX,0
		MOV		ES,AX
		MOV		SI,msg
putloop:
		MOV		AL,[SI]
		ADD		SI,1				
		CMP		AL,0
		JE		fin
		MOV		AH,0x0e			
		MOV		BX,15			
		INT		0x10			;BIOS API
		JMP		putloop
fin:
		HLT						
		JMP		fin				
msg:
		DB		0x0a, 0x0a		
		DB		"load error"
		DB		0x0a			
		DB		0

		RESB	0x7dfe-$		

		DB		0x55, 0xaa

