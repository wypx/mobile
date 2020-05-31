CC      = gcc
STRIP 	= strip  
all:	serialCom

serialCom:	serialCom.o
	$(CC) -o serialCom serialCom.o -lpthread
	#$(STRIP) $@
	$(STRIP) -x -R .note -R .comment $@

serialCom.o:	serialCom.c
	$(CC) -c -o serialCom.o serialCom.c -lpthread

clean:
	rm -f serialCom.o serialCom *~
