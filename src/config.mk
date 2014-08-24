ROSS_SRC=/dcsdata/home/mikida2/ross/src
CHARMC=~/charm/bin/charmc
CC=g++

# Turn on this option to test if code compiles without explicit global variable
# declarations. We should instead use the PE_VALUE macro defined in globals.h.
OPTS+=-DNO_GLOBALS -w -O3

# Turn on this option to test if code compiles without extra forward
# declarations. Once all of the modules are complete, we should no longer need
# forward decls and can just include the appropriate module.
#OPTS+=-DNO_FORWARD_DECLS

BUILD_DIR=~/ross/src/build

MODULES=charm collections ross_setup ross_opts ross_util ross_rand ross_event ross_maps

# Add each module to the include path, as well as the top level directory.
OPTS+=$(addprefix -I$(ROSS_SRC)/, $(MODULES)) -I$(ROSS_SRC)
