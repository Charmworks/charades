include ../config.mk

all: $(BUILD_DIR)/ross_util.o

$(BUILD_DIR)/ross_util.o: ross_util.cpp ross_util.h
	$(CHARMC) $(OPTS) -c ross_util.cpp -o $@
