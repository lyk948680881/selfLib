### file dirs
DIRS         = .

### FOR C
ALLSRC       = $(notdir $(foreach n,$(DIRS),$(wildcard $(n)/*.c)))
KILL         =
SRC          = $(filter-out $(KILL),$(ALLSRC))
OBJS         = $(SRC:%.c=%.o)
DEPS         = $(SRC:%.c=%.d)

### FOR CPP
ALLSRC1      = $(notdir $(foreach n,$(DIRS),$(wildcard $(n)/*.cpp)))
KILL1        =
SRC1         = $(filter-out $(KILL1),$(ALLSRC1))
OBJS1        = $(SRC1:%.cpp=%.o)
DEPS1        = $(SRC1:%.cpp=%.d)
ASM_OBJ      = $(wildcard *.s)

VPATH        = $(DIRS)
LINK_FLAGS   = --sysroot=$(SDK_ROOT) -shared -lstdc++


LIB_NAME     = $(notdir $(CURDIR)).so
OUT_FILE     = $(BUILD_PATH)/$(LIB_NAME)

.PHONY:all clean
all:$(DEPS) $(DEPS1) $(OBJS) $(OBJS1) target
$(OBJS):%.o:
	$(CC) $(CC_FLAGS) $< -o $@

$(OBJS1):%.o:
	$(CC) $(CC_FLAGS) $< -o $@
	

$(DEPS):%.d:%.c
	@echo "CHECK $<"
	@$(CC) -MM $(CC_FLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$
	
$(DEPS1):%.d:%.cpp
	@echo "CHECK $<"
	@$(CC) -MM $(CC_FLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

	
target:	
	$(CC) -o $(OUT_FILE) $(OBJS) $(OBJS1) $(ASM_OBJ) $(LIB_FILE) $(LINK_FLAGS) $(CORE_LIB_FILE)
	
clean:
	@echo "CLEAN $(CURDIR)/ *.d *.o" 
	@for dir in $(DIRS); do \
	$(RM)  $$dir/*.d; \
	$(RM)  $$dir/*.o; \
	done

-include $(DEPS)
-include $(DEPS1)

