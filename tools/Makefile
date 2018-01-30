#############################
# Makefile for pppd chat process
# by luotang
# 2017.7.30
#############################
ROOTDIR = ../../..
include $(ROOTDIR)/Rules.make

DIR_ppp = src/ppp-2.4.7

INCPATH	=$(ROOTDIR)/inc
LIBPATH	=$(ROOTDIR)/lib

all:make_ppp

make_ppp: 
	rm -rf bin/*
	cd $(DIR_ppp)  && chmod -R 777 * && \
	mkdir -p bin && \
	./configure --prefix="$(PWD)/bin" && \
	make clean && \
	make CC=$(CC) && \
	make install 
	#$(STRIP) -x -R .note -R .comment 	bin/sbin/pppd
	#$(STRIP) -x -R .note -R .comment	bin/sbin/chat
clean:
	cd $(DIR_ppp)  && make clean
	rm -rf lib/*
