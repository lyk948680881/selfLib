MAIN_PATH    = $(CURDIR)
FT_PATH      = $(SDK_INC)/freetype2
UDP_PATH     = $(CURDIR)/include/udpsocket
CORE_PATH    = $(CURDIR)/include/core

INCLUDE_PATH = -I. -I$(CURDIR)/include -I$(UDP_PATH) -I$(SDK_INC) -I$(FT_PATH)
INCLUDE_PATH += -I$(CORE_PATH)
CC_FLAGS     = -Wall -fPIC -fno-strict-aliasing -O3  -c $(INCLUDE_PATH)

BUILD_PATH   = $(CURDIR)/build

export  MAIN_PATH
export  BUILD_PATH
export  CC_FLAGS

LIB_DIR = udpsocket
LIB_DIR += core

.PHONY:staticLib clean rmlib
staticLib:path libs 
path:
	mkdir -p $(BUILD_PATH)

libs:
	for dir in $(LIB_DIR); do \
	    $(MAKE) -C $$dir; \
	done
			
clean:	
	for dir in $(LIB_DIR); do \
	    $(MAKE) clean -C $$dir; \
	done
	$(RM) -r $(BUILD_PATH)
	
rmlib:
	$(RM) -r $(BUILD_PATH)	