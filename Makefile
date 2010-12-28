VERSION = 0.7.5
KDIR   := /lib/modules/$(shell uname -r)
KBUILD := $(MAKE) -C $(KDIR)/build M=$(shell pwd)/src
MODULES = src/swkeybd.ko src/swmouse.ko

DEPMOD   = depmod
MODPROBE = modprobe

PHONY += default
default: $(MODULES)

%.ko: %.c src/swinput.h Makefile
	$(KBUILD) modules

PHONY += clean
clean:
	$(KBUILD) clean

PHONY += install
install: $(MODULES)
	-$(MODPROBE) --quiet --remove swkeybd swmouse
	$(KBUILD) modules_install
	$(DEPMOD) --all
	$(MODPROBE) swkeybd
	$(MODPROBE) swmouse
	sleep 1
	chmod a+w /dev/swkeybd /dev/swmouse*

PHONY += uninstall
uninstall:
	-$(MODPROBE) --quiet --remove swkeybd swmouse
	cd $(KDIR)/extra && rm -f $(notdir $(MODULES))
	$(DEPMOD) --all

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
