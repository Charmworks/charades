include ../config.mk

all: $(BUILD_DIR)/avl_tree.o

$(BUILD_DIR)/avl_tree.o: avl_tree.C avl_tree.h
	$(CHARMC) $(OPTS) -c avl_tree.C -o $@
