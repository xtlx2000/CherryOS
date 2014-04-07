flag = -g

mm:main.o buddy.o buffer.o cache.o ext2.o page.o pg_data.o pm.o slab.o vfs.o vm.o zone.o
	gcc  $(flag) -o os main.o buddy.o buffer.o cache.o ext2.o page.o pg_data.o pm.o slab.o vfs.o vm.o zone.o

mian.o:
	gcc  $(flag)  -c main.c

buddy.o:
	gcc  $(flag)  -c buddy.c

buffer.o:
	gcc  $(flag)  -c buffer.c

cache.o:
	gcc  $(flag)  -c cache.c

ext2.o:
	gcc  $(flag)  -c ext2.c

page.o:
	gcc  $(flag)  -c page.c

pg_data.o:
	gcc  $(flag)  -c pg_data.c

pm.o:
	gcc  $(flag)  -c pm.c

slab.o:
	gcc  $(flag)  -c slab.c

vfs.o:
	gcc  $(flag)  -c vfs.c

vm.o:
	gcc  $(flag)  -c vm.c

zone.o:
	gcc  $(flag)  -c zone.c


clean:
	rm os  *.o