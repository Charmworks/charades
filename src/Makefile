include config.mk

C_FILES=$(wildcard *.C)
CI_FILES=$(wildcard *.ci)
MODULES=$(CI_FILES:.ci=.decl.h)
OBJS=$(addprefix $(BUILD_DIR)/, $(C_FILES:.C=.o))

default: all
all: $(BUILD_DIR) $(BUILD_DIR)/libross.a

.PHONY : $(SUB_DIRS)

$(BUILD_DIR):
	mkdir $(BUILD_DIR)

$(BUILD_DIR)/libross.a: make_ci $(OBJS) $(SUB_DIRS)
	$(CHARMC) $(LFLAGS) $(OBJS) $(BUILD_DIR)/*.o -o $@

make_ci: $(MODULES)
	for dir in $(SUB_DIRS); do make -C $${dir} make_ci; done

$(SUB_DIRS):
	make -C $@

%.decl.h: %.ci
	$(CHARMC) $<

$(BUILD_DIR)/%.o: %.C %.h $(MODULES)
	$(CHARMC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.C $(MODULES)
	$(CHARMC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf *.decl.h *.def.h $(OBJS) $(BUILD_DIR)/libross.a
	for dir in $(SUB_DIRS); do make -C $${dir} clean; done
