/*
 * ion/ioncore/mwmhints.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2009.
 *
 * See the included file LICENSE for details.
 */

#include "common.h"
#include "property.h"
#include "mwmhints.h"
#include "global.h"

WMwmHints *xwindow_get_mwmhints(Window win) {
  WMwmHints *hints = NULL;
  int n;

  n = xwindow_get_property(win, ioncore_g.atom_mwm_hints,
                           ioncore_g.atom_mwm_hints, MWM_N_HINTS, FALSE,
                           (uchar **)&hints);

  if (n < MWM_N_HINTS && hints != NULL) {
    XFree((void *)hints);
    return NULL;
  }

  return hints;
}

