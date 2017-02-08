#ifndef ION_IONCORE_STRINGS_H
#define ION_IONCORE_STRINGS_H

#include "common.h"

#include <wchar.h>
#include <wctype.h>

#include "gr.h"

extern bool ioncore_defshortening(const char *rx, const char *rule, bool always);
extern char *grbrush_make_label(GrBrush *brush, const char *str, uint maxw);

extern int str_nextoff(const char *p, int pos);
extern int str_prevoff(const char *p, int pos);
extern int str_len(const char *p);
extern wchar_t str_wchar_at(char *p, int max);
extern char *str_stripws(char *p);

#endif
