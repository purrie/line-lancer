#ifndef UNIT_POOL_H_
#define UNIT_POOL_H_

#include "types.h"

void unit_pool_init ();
void unit_pool_deinit ();

void   unit_release (Unit * unit);
Unit * unit_alloc ();

#endif // UNIT_POOL_H_
