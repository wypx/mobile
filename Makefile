#!/bin/sh
########################################################
#  Makefile for Mobile Process 
#  by luotang
#  2018.1.7
#  project github: https://github.com/wypx/mobile
########################################################
ROOTDIR = ../..
include $(ROOTDIR)/Rules.make

TMP		= bin
TARGET	= $(BINPATH)/msf_mobile

SRCS 	= $(wildcard src/*.c)
OBJS	= $(SRCS:%.c=$(TMP)/%.o)

IFLAGS 		+= -I./inc -I $(ROOTDIR)/inc
IFLAGS	    += -I ../libmsf/msf_thirdparty/cJSON
IFLAGS	    += -I ../libmsf/msf/inc
IFLAGS	    += -I ../libmsf/msf/inc/base
IFLAGS	    += -I ../libmsf/msf/inc/log
IFLAGS	    += -I ../libmsf/msf/inc/system 
IFLAGS	    += -I ../libmsf/msf/inc/timer 
IFLAGS	    += -I ../libmsf/msf/inc/event 
IFLAGS	    += -I ../libmsf/msf/inc/mem 
IFLAGS	    += -I ../libmsf/msf/inc/io  
IFLAGS	    += -I ../libmsf/msf/inc/thread  
IFLAGS	    += -I ../libmsf/msf/inc/encrypt  
IFLAGS	    += -I ../libmsf/msf/inc/svc  
IFLAGS	    += -I ../libmsf/msf/inc/network
IFLAGS 		+= -I ../librpc/comm -I../librpc/client/inc
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