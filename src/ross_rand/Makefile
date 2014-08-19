include ../config.mk

all: $(BUILD_DIR)/ross_random.o $(BUILD_DIR)/ross_clcg4.o

$(BUILD_DIR)/ross_random.o: ross_random.cpp ross_random.h
	$(CHARMC) $(OPTS) -c ross_random.cpp -o $@


$(BUILD_DIR)/ross_clcg4.o: ross_clcg4.cpp ross_clcg4.h
	$(CHARMC) $(OPTS) -c ross_clcg4.cpp -o $@
