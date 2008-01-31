VERSION := 0.7.2
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
	mkdir -p swinput-$(VERSION)/src
	cp   ChangeLog COPYING README  Makefile swinput-$(VERSION)/
	cp   src/*.c swinput-$(VERSION)/src/
	cp   src/Makefile swinput-$(VERSION)/src/
	rm -f swinput-$(VERSION).tar swinput-$(VERSION).tar.gz
	tar cvf swinput-$(VERSION).tar   swinput-$(VERSION)/
	gzip swinput-$(VERSION).tar 
	gpg -b swinput-$(VERSION).tar.gz
