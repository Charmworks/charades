#ifndef GLOBALS_H_
#define GLOBALS_H_

// A struct for holding global variables used by ROSS. An instance of this
// struct will be held by each PE group chare.

struct Globals {
};

// Get the local branch of the PE group and return its globals.
// If possible we should cache the pointer to the local branch.
Globals* get_globals();

#endif
