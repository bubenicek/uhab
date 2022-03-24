
MODULE=vehabus
KERNEL = ~/sources/marsboard/kernel/linux-sunxi-uhab
CROSS_COMPILE=/opt/gcc-arm-linux-gnueabihf-4.7/bin/arm-linux-gnueabihf-

ccflags-y = -I$(PWD)/../../../../../../platform/linux/kernel

obj-m += $(MODULE).o
$(MODULE)-objs := src/vehabus_drv.o 

all:
	make -C ${KERNEL} ARCH=arm CROSS_COMPILE=${CROSS_COMPILE}  M=$(PWD) modules 

clean:
	make -C ${KERNEL} ARCH=mips CROSS_COMPILE=${CROSS_COMPILE} M=$(PWD) clean

load:
	sshpass -p toor	scp vehabus.ko root@192.168.8.102:/opt/uhab
