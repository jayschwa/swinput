VERSION := 0.7.5
KDIR   := /lib/modules/$(shell uname -r)/build
PWD    := $(shell pwd)
KBUILD := $(MAKE) -C $(KDIR) SUBDIRS=$(PWD)/src

PHONY += default
default:
	cd src && $(KBUILD) modules

PHONY += clean
clean:
	cd src && $(KBUILD) clean

PHONY += install
install: default
	cd src && $(KBUILD) modules_install && depmod -a

PHONY += try
try:
	-sudo rmmod swmouse 
	-sudo rmmod swkeybd
	sudo insmod src/swmouse.ko && sudo insmod src/swkeybd.ko
	sleep 1
	sudo chmod a+rw /dev/swkeybd /dev/swmouse0

PHONY += check
check:
	cd test && ./all.sh

PHONY += dist
dist:
	make clean
	mkdir -p swinput-$(VERSION)/
	rm -fr swinput-$(VERSION)/*
	mkdir -p swinput-$(VERSION)/src 
	mkdir -p swinput-$(VERSION)/test
	mkdir -p swinput-$(VERSION)/test/data/swkeybd
	mkdir -p swinput-$(VERSION)/test/data/swmouse
	mkdir -p swinput-$(VERSION)/test/bc
	cp   ChangeLog COPYING README  Makefile swinput-$(VERSION)/
	cp   test/*.sh swinput-$(VERSION)/test/
	cp   test/bc/*.bc swinput-$(VERSION)/test/bc/
	cp   test/data/swkeybd/*.txt swinput-$(VERSION)/test/data/swkeybd
	-cp   test/data/swmouse/*.txt swinput-$(VERSION)/test/data/swmouse
	cp   src/*.c src/*.h swinput-$(VERSION)/src/
	cp   src/Makefile swinput-$(VERSION)/src/
	rm -f swinput-$(VERSION).tar swinput-$(VERSION).tar.gz
	tar cvf swinput-$(VERSION).tar   swinput-$(VERSION)/
	gzip swinput-$(VERSION).tar 
	gpg -b swinput-$(VERSION).tar.gz

.PHONY: $(PHONY)
