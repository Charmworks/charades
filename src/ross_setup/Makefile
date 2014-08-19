include ../config.mk

all: $(BUILD_DIR)/ross_setup.o

$(BUILD_DIR)/ross_setup.o: ross_setup.cpp ross_setup.h
	$(CHARMC) $(OPTS) -c ross_setup.cpp -o $@
