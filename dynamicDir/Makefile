RKAIQ_INC_DIR = $(SDK_INC)/rkaiq
RKAIQ_INC     = -I$(RKAIQ_INC_DIR)/uAPI -I$(RKAIQ_INC_DIR)/algos -I$(RKAIQ_INC_DIR)/xcore -I$(RKAIQ_INC_DIR)/common -I$(RKAIQ_INC_DIR)/iq_parser
RV1126_INC    = -I$(SDK_INC)/easymedia  -I$(SDK_INC)/rga $(RKAIQ_INC)

DYNAMIC_PATH = $(CURDIR)/include
INCLUDE_PATH = $(RV1126_INC) -I$(DYNAMIC_PATH)  -I$(DYNAMIC_PATH)/rv1126 -I$(DYNAMIC_PATH)/core -I$(DYNAMIC_PATH)/drone

CC_FLAGS     = -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -Os -fno-rtti -fvisibility=hidden -fvisibility-inlines-hidden  -DNDEBUG   -Wshadow -ffast-math -fPIC -std=gnu++11 -mfloat-abi=hard -mfpu=neon 
CC_FLAGS    += -Wall -fPIC -fno-strict-aliasing -O3 -c $(INCLUDE_PATH)
	
LIB_FILE     = $(SDK_LIB)/librkaiq.so \
	$(SDK_LIB)/libeasymedia.so \
	$(SDK_LIB)/librga.so 
	
	
export CC_FLAGS 
export LIB_FILE
export BUILD_PATH
export INCLUDE_PATH

BUILD_PATH = $(CURDIR)/build
SUBDIRS  = RV1126
SUBDIRS  += drone

.PHONY:dynamicLib clean rmlib

dynamicLib:path app
path:
	mkdir -p $(BUILD_PATH)

app:
	for dir in $(SUBDIRS); do \
	$(MAKE) -C $$dir; \
	done

clean:		
	for dir in $(SUBDIRS); do \
	$(MAKE) -C $$dir clean ; \
	done
	$(RM) -r $(BUILD_PATH)
	
rmlib:
	$(RM) -r $(BUILD_PATH)


