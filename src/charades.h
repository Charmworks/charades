/** \file charades.h
 *  Single include for models which includes all other required headers.
 *
 *  \todo Get rid of/move the other random declarations and defines as needed.
 */

#ifndef _CHARADES_H
#define _CHARADES_H

#include "command_line.h"
#include "event.h"
#include "factory.h"
#include "globals.h"
#include "lp.h"
#include "mapper.h"
#include "random.h"
#include "setup.h"
#include "util.h"

#include <charm++.h>

#if not USE_CHARMC
void _registerExternalModules(char **argv);
void _createTraces(char **argv);
#endif

#endif
