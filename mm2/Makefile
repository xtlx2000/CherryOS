flag = -g

mm:main.o buddy.o buffer.o cache.o ext2.o page.o pg_data.o pm.o slab.o kmem.o fs.o vfs.o vm.o zone.o radix.o prio_tree.o  rbtree.o
	gcc  $(flag) -o os main.o buddy.o buffer.o cache.o ext2.o page.o pg_data.o pm.o slab.o kmem.o fs.o vfs.o vm.o zone.o radix.o prio_tree.o  rbtree.o

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

kmem.o:
	gcc  $(flag)  -c kmem.c

fs.o:
	gcc  $(flag)  -c fs.c

vfs.o:
	gcc  $(flag)  -c vfs.c

vm.o:
	gcc  $(flag)  -c vm.c

zone.o:
	gcc  $(flag)  -c zone.c

radix.o:
	gcc  $(flag)  -c radix.c

prio_tree.o:
	gcc  $(flag)  -c prio_tree.c

rbtree.o:
	gcc  $(flag)  -c rbtree.c



clean:
	rm os  *.o