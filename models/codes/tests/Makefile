include ../config.mk

TESTS = modelnet-test mapping_test modelnet-test-collective

default: all
all: $(TESTS)

% :: %.cpp $(CODES_BUILD)/libcodes.a $(BUILD_DIR)/libcharades.a
	$(CHARMC) $(CFLAGS) $< -o $@ $(LFLAGS) -L$(CODES_BUILD) -L$(BUILD_DIR) -lcodes -lcharades

#modelnet-test: modelnet-test.cpp $(CODES_BUILD)/libcodes.a $(BUILD_DIR)/libcharades.a
#	$(CHARMC) $(CFLAGS) $(LFLAGS) modelnet-test.cpp -o $@ -L$(BUILD_DIR) -L$(CODES_BUILD) -lcharades -lcodes

#mapping_test: modelnet-test.cpp $(CODES_BUILD)/libcodes.a $(BUILD_DIR)/libcharades.a
#	$(CHARMC) $(CFLAGS) $(LFLAGS) modelnet-test.cpp -o $@ -L$(BUILD_DIR) -L$(CODES_BUILD) -lcharades -lcodes

test: $(TESTS)
	for test in $(TESTS); do ./$${test}.sh; done

clean:
	rm -rf $(TESTS) charmrun
