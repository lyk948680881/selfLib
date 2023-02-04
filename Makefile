SDK_ROOT     = /home/new/Software/rv1126/rv1126-sdk/sysroot
SDK_INC      = $(SDK_ROOT)/usr/include
SDK_LIB      = $(SDK_ROOT)/usr/lib

GCC_DIR      = $(SDK_ROOT)/../gcc-linaro-6.3.1-2017.05-x86_64_arm-linux-gnueabihf/bin

CROSS        = $(GCC_DIR)/arm-linux-gnueabihf-
CC           = $(CROSS)gcc
AR           = $(CROSS)ar cqs
AS           = $(CROSS)as
STRIP        = $(CROSS)strip

#TEST

export  CC
export  AR
export  AS

export  SDK_INC
export  SDK_LIB
export  SDK_ROOT

#内核库,rv1126 需要内核的内存池接口
CORE_LIB_FILE = $(CURDIR)/staticDir/build/core.a
export CORE_LIB_FILE

STATICDIRS = staticDir 
DYNAMICDIRS = dynamicDir 
SUBDIRS = staticDir dynamicDir

.PHONY:all staticlib dynamiclib cleanD cleanS rmlib clean

all:
	for dir in $(SUBDIRS); do \
	$(MAKE) -C $$dir ;\
	done

staticlib:
	for dir in $(STATICDIRS); do \
	$(MAKE) -C $$dir ;\
	done

dynamiclib:
	for dir in $(DYNAMICDIRS); do \
	$(MAKE) -C $$dir ;\
	done

cleanD:	
	for dir in $(DYNAMICDIRS); do \
	$(MAKE) clean -C $$dir ;\
	done
cleanS:	
	for dir in $(STATICDIRS); do \
	$(MAKE) clean -C $$dir ;\
	done
rmlib:
	for dir in $(SUBDIRS); do \
	$(MAKE) rmlib -C $$dir ;\
	done
clean:	
	for dir in $(SUBDIRS); do \
	$(MAKE) clean -C $$dir ;\
	done


