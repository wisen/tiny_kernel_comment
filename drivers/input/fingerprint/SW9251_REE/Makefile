

ccflags-y +=-I$(srctree)/drivers/spi/mediatek/$(MTK_PLATFORM)

MODFLAGS       =$(MODFLAGS)  -DMODULE -fno-pic  

sunwave-objs := sunwavecorp.o   platform.o misce.o
sunwave-objs +=  mt6797_dts.o
obj-y   = sunwave.o mt6797_dts_plat.o

