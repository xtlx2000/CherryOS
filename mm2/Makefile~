mm:main.o buddy.o buffer.o cache.o ext2.o page.o pg_data.o pm.o slab.o vfs.o vm.o zone.o
	gcc -o os main.o buddy.o buffer.o cache.o ext2.o page.o pg_data.o pm.o slab.o vfs.o vm.o zone.o

mian.o:
	gcc -c main.c


buddy.o:buddy.c
	gcc -c buddy.c

buffer.o:
	gcc -c buffer.c

cache.o:
	gcc -c cache.c

ext2.o:
	gcc -c ext2.c

page.o:
	gcc -c page.c

pg_data.o:
	gcc -c pg_data.c

pm.o:
	gcc -c pm.c

slab.o:
	gcc -c slab.c

vfs.o:
	gcc -c vfs.c

vm.o:
	gcc -c vm.c

zone.o:
	gcc -c zone.c


clean:
	rm os  *.o