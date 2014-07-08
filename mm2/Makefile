OBJS_BOOTPACK = kernel.obj interaction.obj hankaku.obj graphic.obj descriptor.obj \
		int.obj pipe.obj keyboard.obj mouse.obj memory.obj coverage.obj timer.obj \
		sched.obj window.obj terminal.obj file.obj

TOOLPATH = ../z_tools/
INCPATH  = ../z_tools/haribote/

MAKE     = $(TOOLPATH)make.exe -r
NASK     = $(TOOLPATH)nask.exe
CC1      = $(TOOLPATH)cc1.exe -I$(INCPATH) -Os -Wall -quiet
GAS2NASK = $(TOOLPATH)gas2nask.exe -a
OBJ2BIM  = $(TOOLPATH)obj2bim.exe
MAKEFONT = $(TOOLPATH)makefont.exe
BIN2OBJ  = $(TOOLPATH)bin2obj.exe
BIM2HRB  = $(TOOLPATH)bim2hrb.exe
RULEFILE = $(TOOLPATH)haribote/haribote.rul
EDIMG    = $(TOOLPATH)edimg.exe
IMGTOL   = $(TOOLPATH)imgtol.com
COPY     = copy
DEL      = rm
FLAG = -I./

KERNELDIR = ./kernel/
INCLUDEDIR = ./include/

#

default :
	$(MAKE) img

# 
fat.bin : $(KERNELDIR)fat.nas Makefile
	$(NASK) $(KERNELDIR)fat.nas fat.bin fat.lst 

boot.bin : boot.nas Makefile
	$(NASK) boot.nas boot.bin boot.lst 

hankaku.bin : hankaku.txt Makefile
	$(MAKEFONT) hankaku.txt hankaku.bin 

hankaku.obj : hankaku.bin Makefile
	$(BIN2OBJ) hankaku.bin hankaku.obj _hankaku 

kernel.bim : $(OBJS_BOOTPACK) Makefile
	$(OBJ2BIM) @$(RULEFILE) out:kernel.bim stack:3136k map:kernel.map \
		$(OBJS_BOOTPACK) 
# 3MB+64KB=3136KB

kernel.hrb : kernel.bim Makefile
	$(BIM2HRB) kernel.bim kernel.hrb 0 

cherryOS.sys : boot.bin kernel.hrb Makefile
	$(COPY) /B boot.bin+kernel.hrb cherryOS.sys 


cherryOS.img : fat.bin cherryOS.sys Makefile 
	$(EDIMG)   imgin:../z_tools/fdimg0at.tek \
		wbinimg src:fat.bin len:512 from:0 to:0 \
		copy from:cherryOS.sys to:@: \
		imgout:cherryOS.img 

# 

%.gas : $(KERNELDIR)%.c $(INCLUDEDIR)kernel.h Makefile
	$(CC1) -o $*.gas $(KERNELDIR)$*.c $(FLAG)

$(KERNELDIR)%.nas : %.gas Makefile
	$(GAS2NASK) $*.gas $(KERNELDIR)$*.nas 

%.obj : $(KERNELDIR)%.nas Makefile
	$(NASK) $(KERNELDIR)$*.nas $*.obj $*.lst 

# 

img :
	$(MAKE) cherryOS.img

run :
	$(MAKE) img
	$(COPY) cherryOS.img ..\z_tools\qemu\fdimage0.bin
	$(MAKE) -C ../z_tools/qemu

install :
	$(MAKE) img
	$(IMGTOL) w a: cherryOS.img

clean :
	-$(DEL) *.bin
	-$(DEL) *.lst
	-$(DEL) *.obj
	-$(DEL) *.map
	-$(DEL) *.bim
	-$(DEL) *.hrb
	-$(DEL) cherryOS.sys
	-$(DEL) cherryOS.img

src_only :
	$(MAKE) clean
	-$(DEL) cherryOS.img
	
