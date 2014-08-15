include config.mk

all: $(MODULES)

.PHONY: $(MODULES)
$(MODULES):
	make -C $@

clean:
	$(foreach module,$(MODULES),make -C $(module) clean;)
