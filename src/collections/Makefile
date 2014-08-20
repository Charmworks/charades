include ../config.mk

all: $(BUILD_DIR)/avl_tree.o $(BUILD_DIR)/pe_queue.o

$(BUILD_DIR)/avl_tree.o: avl_tree.C avl_tree.h
	$(CHARMC) $(OPTS) -c $< -o $@

$(BUILD_DIR)/pe_queue.o: pe_queue.C pe_queue.h
	$(CHARMC) $(OPTS) -c $< -o $@

clean:
	rm $(BUILD_DIR)/avl_tree.o $(BUILD_DIR)/pe_queue.o
