include ../config.mk

TESTS = model-net-synthetic-fattree model-net-synthetic-slimfly model-net-synthetic-dragonfly

default: all
all: $(TESTS)

% :: %.cpp $(CODES_BUILD)/libcodes.a $(BUILD_DIR)/libcharades.a
	$(CHARMC) $(CFLAGS) $< -o $@ $(LFLAGS) -L$(CODES_BUILD) -L$(BUILD_DIR) -lcodes -lcharades

clean:
	rm -rf $(TESTS) charmrun
