/*
 * ion/ioncore/conf.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2009.
 *
 * See the included file LICENSE for details.
 */

#include <stdlib.h>
#include <string.h>

#include <libtu/map.h>
#include <libtu/minmax.h>
#include <libtu/objp.h>
#include <libtu/map.h>
#include <libextl/readconfig.h>

#include "common.h"
#include "global.h"
#include "modules.h"
#include "rootwin.h"
#include "bindmaps.h"
#include "kbresize.h"
#include "reginfo.h"
#include "group-ws.h"
#include "llist.h"

StringIntMap frame_idxs[] = {{"last", LLIST_INDEX_LAST},
                             {"next", LLIST_INDEX_AFTER_CURRENT},
                             {"next-act", LLIST_INDEX_AFTER_CURRENT_ACT},
                             END_STRINGINTMAP};

StringIntMap win_stackrq[] = {
    {"ignore", IONCORE_WINDOWSTACKINGREQUEST_IGNORE},
    {"activate", IONCORE_WINDOWSTACKINGREQUEST_ACTIVATE},
    END_STRINGINTMAP};

static bool get_winprop_fn_set = FALSE;
static ExtlFn get_winprop_fn;

static bool get_layout_fn_set = FALSE;
static ExtlFn get_layout_fn;

EXTL_EXPORT
void ioncore_set(ExtlTab tab) {
  int dd;
  char *tmp;
  ExtlFn fn;

  extl_table_gets_b(tab, "opaque_resize", &(ioncore_g.opaque_resize));
  extl_table_gets_b(tab, "warp", &(ioncore_g.warp_enabled));
  extl_table_gets_b(tab, "switchto", &(ioncore_g.switchto_new));
  extl_table_gets_b(tab, "screen_notify", &(ioncore_g.screen_notify));
  extl_table_gets_b(tab, "framed_transients", &(ioncore_g.framed_transients));
  extl_table_gets_b(tab, "unsqueeze", &(ioncore_g.unsqueeze_enabled));
  extl_table_gets_b(tab, "autoraise", &(ioncore_g.autoraise));
  extl_table_gets_b(tab, "autosave_layout", &(ioncore_g.autosave_layout));

  if (extl_table_gets_s(tab, "window_stacking_request", &tmp)) {
    ioncore_g.window_stacking_request =
        stringintmap_value(win_stackrq, tmp, ioncore_g.window_stacking_request);
    free(tmp);
  }

  if (extl_table_gets_s(tab, "frame_default_index", &tmp)) {
    ioncore_g.frame_default_index =
        stringintmap_value(frame_idxs, tmp, ioncore_g.frame_default_index);
    free(tmp);
  }

  if (extl_table_gets_s(tab, "mousefocus", &tmp)) {
    if (strcmp(tmp, "disabled") == 0)
      ioncore_g.no_mousefocus = TRUE;
    else if (strcmp(tmp, "sloppy") == 0)
      ioncore_g.no_mousefocus = FALSE;
    free(tmp);
  }

  if (extl_table_gets_i(tab, "dblclick_delay", &dd))
    ioncore_g.dblclick_delay = maxof(0, dd);

  if (extl_table_gets_i(tab, "usertime_diff_current", &dd))
    ioncore_g.usertime_diff_current = maxof(0, dd);

  if (extl_table_gets_i(tab, "usertime_diff_new", &dd))
    ioncore_g.usertime_diff_new = maxof(0, dd);

  if (extl_table_gets_i(tab, "focuslist_insert_delay", &dd))
    ioncore_g.focuslist_insert_delay = maxof(0, dd);

  if (extl_table_gets_i(tab, "workspace_indicator_timeout", &dd))
    ioncore_g.workspace_indicator_timeout = maxof(0, dd);

  extl_table_gets_b(tab, "activity_notification_on_all_screens",
                    &(ioncore_g.activity_notification_on_all_screens));

  ioncore_set_moveres_accel(tab);

  ioncore_groupws_set(tab);

  /* Internal -- therefore undocumented above */
  if (extl_table_gets_f(tab, "_get_winprop", &fn)) {
    if (get_winprop_fn_set) extl_unref_fn(get_winprop_fn);
    get_winprop_fn = fn;
    get_winprop_fn_set = TRUE;
  }

  if (extl_table_gets_f(tab, "_get_layout", &fn)) {
    if (get_layout_fn_set) extl_unref_fn(get_layout_fn);
    get_layout_fn = fn;
    get_layout_fn_set = TRUE;
  }
}

EXTL_SAFE
EXTL_EXPORT
ExtlTab ioncore_get() {
  ExtlTab tab = extl_create_table();

  extl_table_sets_b(tab, "opaque_resize", ioncore_g.opaque_resize);
  extl_table_sets_b(tab, "warp", ioncore_g.warp_enabled);
  extl_table_sets_b(tab, "switchto", ioncore_g.switchto_new);
  extl_table_sets_i(tab, "dblclick_delay", ioncore_g.dblclick_delay);
  extl_table_sets_b(tab, "screen_notify", ioncore_g.screen_notify);
  extl_table_sets_b(tab, "framed_transients", ioncore_g.framed_transients);
  extl_table_sets_b(tab, "unsqueeze", ioncore_g.unsqueeze_enabled);
  extl_table_sets_b(tab, "autoraise", ioncore_g.autoraise);
  extl_table_sets_b(tab, "autosave_layout", ioncore_g.autosave_layout);
  extl_table_sets_i(tab, "focuslist_insert_delay",
                    ioncore_g.focuslist_insert_delay);
  extl_table_sets_i(tab, "workspace_indicator_timeout",
                    ioncore_g.workspace_indicator_timeout);
  extl_table_sets_b(tab, "activity_notification_on_all_screens",
                    ioncore_g.activity_notification_on_all_screens);

  extl_table_sets_s(
      tab, "window_stacking_request",
      stringintmap_key(win_stackrq, ioncore_g.window_stacking_request, NULL));

  extl_table_sets_s(
      tab, "frame_default_index",
      stringintmap_key(frame_idxs, ioncore_g.frame_default_index, NULL));

  extl_table_sets_s(tab, "mousefocus",
                    (ioncore_g.no_mousefocus ? "disabled" : "sloppy"));

  ioncore_get_moveres_accel(tab);

  ioncore_groupws_get(tab);

  return tab;
}

ExtlTab ioncore_get_winprop(WClientWin *cwin) {
  ExtlTab tab = extl_table_none();

  if (get_winprop_fn_set) {
    extl_protect(NULL);
    extl_call(get_winprop_fn, "o", "t", cwin, &tab);
    extl_unprotect(NULL);
  }

  return tab;
}

ExtlTab ioncore_get_layout(const char *layout) {
  ExtlTab tab = extl_table_none();

  if (get_layout_fn_set) {
    extl_protect(NULL);
    extl_call(get_layout_fn, "s", "t", layout, &tab);
    extl_unprotect(NULL);
  }

  return tab;
}

EXTL_SAFE
EXTL_EXPORT
ExtlTab ioncore_get_paths(ExtlTab tab) {
  tab = extl_create_table();
  extl_table_sets_s(tab, "userdir", extl_userdir());
  extl_table_sets_s(tab, "sessiondir", extl_sessiondir());
  extl_table_sets_s(tab, "searchpath", extl_searchpath());
  return tab;
}

EXTL_EXPORT
bool ioncore_set_paths(ExtlTab tab) {
  char *s;

  if (extl_table_gets_s(tab, "userdir", &s)) {
    warn("User directory can not be set.");
    free(s);
    return FALSE;
  }

  if (extl_table_gets_s(tab, "sessiondir", &s)) {
    extl_set_sessiondir(s);
    free(s);
    return FALSE;
  }

  if (extl_table_gets_s(tab, "searchpath", &s)) {
    extl_set_searchpath(s);
    free(s);
    return FALSE;
  }

  return TRUE;
}

EXTL_SAFE
EXTL_EXPORT_AS(ioncore, lookup_script)
char *extl_lookup_script(const char *file, const char *sp);

EXTL_SAFE
EXTL_EXPORT_AS(ioncore, get_savefile)
char *extl_get_savefile(const char *basename);

EXTL_SAFE
EXTL_EXPORT_AS(ioncore, write_savefile)
bool extl_write_savefile(const char *basename, ExtlTab tab);

EXTL_SAFE
EXTL_EXPORT_AS(ioncore, read_savefile)
ExtlTab extl_extl_read_savefile(const char *basename);

bool ioncore_read_main_config(const char *cfgfile) {
  bool ret;
  int unset = 0;

  if (cfgfile == NULL) cfgfile = "cfg_notion";

  ret = extl_read_config(cfgfile, ".", TRUE);

  unset += (ioncore_screen_bindmap->nbindings == 0);
  unset += (ioncore_mplex_bindmap->nbindings == 0);
  unset += (ioncore_frame_bindmap->nbindings == 0);

  if (unset > 0) {
    warn("Some bindmaps were empty, loading ioncore_efbb.");
    extl_read_config("ioncore_efbb", NULL, TRUE);
  }

  return (ret && unset == 0);
}
