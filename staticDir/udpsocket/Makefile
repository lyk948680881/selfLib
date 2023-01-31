### file dirs
DIRS         = . src

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

LIB_FILE     = 
LINK_FLAGS   = 
LIB_NAME 	 = $(notdir $(CURDIR)).a
OUT_FILE     = $(BUILD_PATH)/$(LIB_NAME) 

.PHONY:all clean target

all:$(DEPS) $(DEPS1) $(OBJS) $(OBJS1) target

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

$(OBJS):%.o:
	$(CC) $(CC_FLAGS) $< -o $@

$(OBJS1):%.o:
	$(CC) $(CC_FLAGS) $< -o $@

target:	
	$(AR) $(OUT_FILE) $(OBJS) $(OBJS1) $(LIB_FILE)  $(LINK_FLAGS)
	
clean:
	@echo "CLEAN $(CURDIR)/ *.d *.o"
	@for dir in $(DIRS); do \
	$(RM)  $$dir/*.d; \
	$(RM)  $$dir/*.o; \
	done

-include $(DEPS)
-include $(DEPS1)

