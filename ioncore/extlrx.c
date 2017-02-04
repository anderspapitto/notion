#include "common.h"

EXTL_SAFE
EXTL_UNTRACED
EXTL_EXPORT
void ioncore_warn(const char *str) { warn("%s", str); }

EXTL_SAFE
EXTL_EXPORT
void ioncore_warn_traced(const char *str) { warn("%s", str); }

EXTL_SAFE
EXTL_EXPORT
const char *ioncore_gettext(const char *s) {
  if (s == NULL)
    return NULL;
  else
    return s;
}
