KDIR   := /lib/modules/$(shell uname -r)/build
PWD    := $(shell pwd)
KERNEL := $(shell uname -r | sed 's,-[0-9\-]*,,g')


default:
	cd src && $(MAKE) -C $(KDIR) SUBDIRS=$(PWD)/src modules 

clean:
	cd src && make clean

install:
	cd src && sudo $(MAKE) -C $(KDIR) SUBDIRS=$(PWD)/src modules modules_install && sudo depmod -a
