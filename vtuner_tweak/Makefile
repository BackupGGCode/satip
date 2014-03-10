#
# Makefile for the vtunerc device driver
#

VTUNERC_MAX_ADAPTERS ?= 4

vtunerc-objs = vtunerc_main.o vtunerc_ctrldev.o vtunerc_proxyfe.o

CONFIG_DVB_VTUNERC ?= m

obj-$(CONFIG_DVB_VTUNERC) += vtunerc.o

ccflags-y += -Idrivers/media/dvb-core
ccflags-y += -Idrivers/media/dvb/dvb-core
ccflags-y += -Idrivers/media/dvb/frontends
ccflags-y += -Idrivers/media/common/tuners
ccflags-y += -Iinclude
ccflags-y += -DVTUNERC_MAX_ADAPTERS=$(VTUNERC_MAX_ADAPTERS)

ifneq "$(ARCH)" ""
	MKARCH = ARCH=$(ARCH)
endif

ifneq "$(CROSS_COMPILE)" ""
	MKCROSS = CROSS_COMPILE=$(CROSS_COMPILE)
endif


#
# for external compilation
#
ifneq "$(KSRC)" ""
	KDIR = $(KSRC)
endif

#KDIR ?= /usr/src/`uname -r`
ifeq ($(origin KDIR), undefined)
	KDIR = /lib/modules/$(shell uname -r)/build
	ifeq "$(wildcard $(KDIR) )" ""
		KDIR = /usr/src/$(shell uname -r)
	endif
	ifeq "$(wildcard $(KDIR) )" ""
		KDIR = /usr/src/kernels/$(shell uname -r)
	endif
endif


default:
	$(MAKE) $(MKARCH) $(MKCROSS) -C $(KDIR) M=$(shell pwd) modules

clean:
	rm -f *.o
	rm -f *.ko
	rm -f *.mod.c
	rm -f .*.cmd
	rm -f *~
	rm -fr .tmp_versions
	rm -f modules.order Module.symvers
 
