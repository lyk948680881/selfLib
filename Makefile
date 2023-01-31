SDK_ROOT     = /home/new/Software/rv1126/rv1126-sdk/sysroot
SDK_INC      = $(SDK_ROOT)/usr/include
SDK_LIB      = $(SDK_ROOT)/usr/lib

GCC_DIR      = $(SDK_ROOT)/../gcc-linaro-6.3.1-2017.05-x86_64_arm-linux-gnueabihf/bin

CROSS        = $(GCC_DIR)/arm-linux-gnueabihf-
CC           = $(CROSS)gcc
AR           = $(CROSS)ar cqs
AS           = $(CROSS)as
STRIP        = $(CROSS)strip

export  CC
export  AR
export  AS

export  SDK_INC
export  SDK_LIB
export  SDK_ROOT

#内核库,rv1126 需要加载入内核的线程池
CORE_LIB_FILE = $(CURDIR)/staticDir/build/core.a
export CORE_LIB_FILE

SUBDIRS = staticDir 
SUBDIRS += dynamicDir 

.PHONY:staticlib clean rmlib

staticlib:
	for dir in $(SUBDIRS); do \
	$(MAKE) -C $$dir ;\
	done

clean:	
	for dir in $(SUBDIRS); do \
	$(MAKE) clean -C $$dir ;\
	done

rmlib:
	for dir in $(SUBDIRS); do \
	$(MAKE) rmlib -C $$dir ;\
	done
