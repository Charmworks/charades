include ../config.mk

C_FILES=$(wildcard *.C)
CI_FILES=$(wildcard *.ci)
MODULES=$(CI_FILES:.ci=.decl.h)
OBJS=$(addprefix $(BUILD_DIR)/, $(C_FILES:.C=.o))

default: all
all: $(BUILD_DIR) $(OBJS)

$(BUILD_DIR):
	mkdir $(BUILD_DIR)

make_ci: $(MODULES)

%.decl.h: %.ci
	$(CHARMC) $<

$(BUILD_DIR)/%.o: %.C %.h $(MODULES)
	$(CHARMC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.C $(MODULES)
	$(CHARMC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf *.decl.h *.def.h
