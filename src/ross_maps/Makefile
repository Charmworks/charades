include ../config.mk

all: $(BUILD_DIR)/ross_block.o

$(BUILD_DIR)/ross_block.o: ross_block.cpp ross_block.h
	$(CHARMC) $(OPTS) -c ross_block.cpp -o $@
