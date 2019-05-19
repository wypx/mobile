#
# Makefile for Mobile Process 
# by luotang 2017.6.1
#

ROOTDIR = ../..
include $(ROOTDIR)/Rules.make

TMP		= bin
TARGET	= $(BINPATH)/msf_mobile

SRCS 	= $(wildcard src/*.c)
OBJS	= $(SRCS:%.c=$(TMP)/%.o)

IFLAGS 		= -I./inc -I $(ROOTDIR)/inc -I../libmsf/msf/inc -I../librpc/comm -I../librpc/client/inc
CFLAGS 		= -Wall -Wextra -W -O2 -pipe -Wswitch-default -Wpointer-arith -g -Wno-unused -ansi -ftree-vectorize -std=gnu11
DFLAGS 		=  
LD_FLAGS 	= -lpthread -ldl -lnuma -L$(LIBPATH) -lmsf -lipc

GFLAG 		= $(CFLAGS) $(DFLAGS) $(IFLAGS)

.PHONY: all clean

all : $(TARGET)

$(TARGET): $(OBJS)
	@echo $(LINK) $@
	@$(LINK) -o $@ $^ $(LD_FLAGS) 
	@echo $(STRIP) $@
	@$(STRIP) -x -R .note -R .comment $@
	@rm -rf $(TMP)
	@echo "====Makefile GFLAG==="
	@echo $(GFLAG)
	@echo "======================"
$(TMP)/%.o:%.c
	@mkdir -p $(BINPATH)
	@mkdir -p $(dir $@)
	@echo $(CC) $@
	@$(CC) $(GFLAG) -c $^ -o $@

clean :
	rm -rf $(TMP)
	
	
#树莓派4G驱动编译：
#usbserial.ko 
#usb_wwan.ko
#option.ko
#export KERNEL_SOURCE_DIR=/home/tomato/raspberrypi/linux-rpi-4.14.y
#make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- -C $KERNEL_SOURCE_DIR M=drivers/usb/serial modules