include config.mk

default: all
all: $(CODES_BUILD) $(CODES_BUILD)/libcodes.a

.PHONY : $(CODES_DIRS)

$(CODES_BUILD):
	mkdir $(CODES_BUILD)

$(CODES_BUILD)/libcodes.a: $(CODES_DIRS)
	$(CHARMC) $(LFLAGS) $(CODES_BUILD)/*.o -o $@

$(CODES_DIRS):
	make -C $@

clean:
	rm -rf $(CODES_BUILD)/libcodes.a
	for dir in $(CODES_DIRS); do make -C $${dir} clean; done
