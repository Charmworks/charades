include config.mk

all: $(BUILD_DIR)/libross.a

.PHONY : $(MODULES)

$(BUILD_DIR)/libross.a: $(MODULES)
	for dir in $(MODULES); do make -C $${dir}; done
	$(CHARMC) -module RotateLB -o $@ $(BUILD_DIR)/*.o
	
$(MODULES):
	make -C $@

clean:
	rm -rf build/*;
	for dir in $(MODULES); do make -C $${dir} clean; done
