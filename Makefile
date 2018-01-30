#
# Makefile for Mobile Process 
# by luotang 2017.6.1
#

ROOTDIR = ../..
include $(ROOTDIR)/Rules.make

BIN		= bin
TARGET	= $(BIN)/mobile

SRCS 	= $(wildcard src/*.c)
OBJS	= $(SRCS:%.c=$(BIN)/%.o)

IFLAGS 		= -I./inc -I $(ROOTDIR)/inc 
CFLAGS 		= -Wall -W -O2 -Wswitch-default -Wpointer-arith -Wno-unused -g
DFLAGS 		= -D_GNU_SOURCE 
LD_FLAGS 	= -lpthread 

GFLAG 		= $(CFLAGS) $(DFLAGS) $(IFLAGS)

.PHONY: all clean

all : $(TARGET)

$(TARGET): $(OBJS)
	$(LINK) -o $@ $^ $(LD_FLAGS) 
	$(STRIP) $@
	$(STRIP) -x -R .note -R .comment $@
	rm -rf $(BIN)/src/
	@echo "====Makefile GFLAG==="
	@echo $(GFLAG)
	@echo "======================"
$(BIN)/%.o:%.c
	@mkdir -p $(BIN)
	@mkdir -p $(dir $@)
	$(CC) $(GFLAG) -c $^ -o $@

clean :
	rm -rf $(BIN)/*