include ../config.mk

all: $(BUILD_DIR)/ross_event.o

$(BUILD_DIR)/ross_event.o: ross_event.cpp ross_event.h
	$(CHARMC) $(OPTS) -c ross_event.cpp -o $@
