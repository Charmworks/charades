#include "globals.h"
#include "pe.h"

// TODO: Should this be a readonly? It can't really be stored in the globals
// struct if it is used to access it.
extern CProxy_PE pes;

Globals* get_globals() {
  return pes.ckLocalBranch()->globals;
}
