include ../config.mk

all: $(BUILD_DIR)/ross_opts.o

$(BUILD_DIR)/ross_opts.o: ross_opts.cpp ross_opts.h
	$(CHARMC) $(OPTS) -c ross_opts.cpp -o $@
