NEW_INPUT_API := $(shell uname -r | sed 's,-[0-9\-]*,,g' | grep -c -e '2.6.1[56789]')

ifeq ($(NEW_INPUT_API),1)
CFLAGS=-D$(NEW_INPUT_API) 
endif

obj-m := swmouse.o swkeybd.o 

module-objs := swmouse.o swkeybd.o 


clean:
	rm -f  *.o *.ko *.mod.c