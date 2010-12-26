VERSION := 0.7.5
KDIR   := /lib/modules/$(shell uname -r)/build
KBUILD := $(MAKE) -C $(KDIR) SUBDIRS=$(shell pwd)/src

PHONY += default
default: build

PHONY += build
build:
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
	rm -rf swinput-$(VERSION)/*
	mkdir -p swinput-$(VERSION)/src 
	mkdir -p swinput-$(VERSION)/test
	mkdir -p swinput-$(VERSION)/test/data/swkeybd
	mkdir -p swinput-$(VERSION)/test/data/swmouse
	mkdir -p swinput-$(VERSION)/test/bc
	cp COPYING README Makefile swinput-$(VERSION)/
	cp test/*.sh swinput-$(VERSION)/test/
	cp test/bc/*.bc swinput-$(VERSION)/test/bc/
	cp test/data/swkeybd/*.txt swinput-$(VERSION)/test/data/swkeybd
	-cp test/data/swmouse/*.txt swinput-$(VERSION)/test/data/swmouse
	cp src/*.c src/*.h swinput-$(VERSION)/src/
	cp src/Makefile swinput-$(VERSION)/src/
	rm -f swinput-$(VERSION).tar.gz
	tar czvf swinput-$(VERSION).tar.gz swinput-$(VERSION)/
	-gpg -b swinput-$(VERSION).tar.gz

.PHONY: $(PHONY)
