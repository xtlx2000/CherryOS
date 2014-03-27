helloworld : main.o print.o
	gcc -o helloworld main.o print.o
	
mian.o : mian.c print.h
	gcc -c main.c
	
print.o : print.c print.h
	gcc -c print.c
	
clean :　　　　　　　　　　
	rm helloworld main.o print.o