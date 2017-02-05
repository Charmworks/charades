include ../config.mk

all: $(BUILD_DIR)/pe.o $(BUILD_DIR)/gvtmanager.o $(BUILD_DIR)/lp.o $(BUILD_DIR)/event.o $(BUILD_DIR)/ross.o $(BUILD_DIR)/lp_pup.o $(BUILD_DIR)/event_pup.o $(BUILD_DIR)/statistics.o

ci_files: ross.ci pe.ci lp.ci event.ci gvtmanager.ci
	$(CHARMC) ross.ci pe.ci lp.ci event.ci gvtmanager.ci
	touch ci_files

$(BUILD_DIR)/ross.o: ci_files ross.h ross.C
	$(CHARMC) $(OPTS) -c ross.C -o $@

$(BUILD_DIR)/pe.o: ci_files pe.h pe.C
	$(CHARMC) $(OPTS) -c pe.C -o $@

$(BUILD_DIR)/gvtmanager.o: ci_files gvtmanager.h gvtmanager.C
	$(CHARMC) $(OPTS) -c gvtmanager.C -o $@

$(BUILD_DIR)/lp.o: ci_files lp.h lp.C
	$(CHARMC) $(OPTS) -c lp.C -o $@

$(BUILD_DIR)/lp_pup.o: ci_files lp.h lp_pup.C
	$(CHARMC) $(OPTS) -c lp_pup.C -o $@

$(BUILD_DIR)/event.o: ci_files event.h event.C
	$(CHARMC) $(OPTS) -c event.C -o $@

$(BUILD_DIR)/event_pup.o: ci_files event.h event_pup.C
	$(CHARMC) $(OPTS) -c event_pup.C -o $@

$(BUILD_DIR)/statistics.o: statistics.h statistics.C
	$(CHARMC) $(OPTS) -c statistics.C -o $@

clean:
	rm -rf *.decl.h *.def.h ci_files
