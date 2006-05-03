VERSION := 0.6
KDIR   := /lib/modules/$(shell uname -r)/build
PWD    := $(shell pwd)
KERNEL := $(shell uname -r | sed 's,-[0-9\-]*,,g')


default:
	cd src && $(MAKE) -C $(KDIR) SUBDIRS=$(PWD)/src modules 

clean:
	cd src && make clean

install:
	cd src && $(MAKE) -C $(KDIR) SUBDIRS=$(PWD)/src modules modules_install && depmod -a

dist:
	make clean
	mkdir -p swinput-$(VERSION)/
	rm -fr swinput-$(VERSION)/*
	cp  src/*.c ChangeLog COPYING README  swinput-$(VERSION)/
	rm -f swinput-$(VERSION).tar swinput-$(VERSION).tar.gz
	tar cvf swinput-$(VERSION).tar   swinput-$(VERSION)/
	gzip swinput-$(VERSION).tar 

