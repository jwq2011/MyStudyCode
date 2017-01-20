# 
# Makefile for ATC3360 bkl
#
#
PWD=$(shell pwd)
#EXTRA_CFLAGS := -O0 -g
EXTRA_CFLAGS := -I$(src)/
EXTRA_CFLAGS := -I$(src)/gfx_hal/
EXTRA_CFLAGS += -I$(DRIVERS_INC)/
EXTRA_CFLAGS += -I$(KERNELDIR)/arch/arm/mach-ac83xx/include/mach/
EXTRA_CFLAGS += -I$(KERNELDIR)/include/generated/
EXTRA_CFLAGS += -I$(src)/../../../../protect/platform/kernel/drivers/libosal/inc
EXTRA_CFLAGS += -I$(src)/../../../../protect/platform/kernel/drivers/private_inc
EXTRA_CFLAGS += -I$(src)/../../../../protect/platform/kernel/drivers/oal
EXTRA_CFLAGS += -DCHIP_VER_AC83XX


obj-m := mic.o

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
	cp -f  mic.ko $(DRIVERS_OUTPUT)

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
