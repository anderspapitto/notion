#include "private.h"

bool extl_obj_error(int ndx, const char *got, const char *wanted) {
  extl_warn("Type checking failed in level 2 call handler for parameter %d (got %s, expected %s).", ndx, got ? got : "nil", wanted);
  return FALSE;
}
