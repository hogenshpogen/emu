ifneq ($(KERNELRELEASE),)
obj-m	:= eth_hook_3_vme.o

else
KDIR	:= /lib/modules/$(shell uname -r)/build
PWD	:= $(shell pwd)

all:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

clean:
	rm -fr .tmp_versions *.o *.ko *.ko.unsigned *.mod *.mod.c .*.cmd Module.markers Module.symvers modules.order

endif
