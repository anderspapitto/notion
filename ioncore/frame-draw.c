#include <string.h>

#include <libtu/objp.h>
#include <libtu/minmax.h>
#include <libtu/map.h>

#include "common.h"
#include "frame.h"
#include "framep.h"
#include "frame-draw.h"
#include "strings.h"
#include "activity.h"
#include "names.h"
#include "gr.h"
#include "gr-util.h"

#define BAR_INSIDE_BORDER(FRAME) \
  ((FRAME)->barmode == FRAME_BAR_INSIDE || (FRAME)->barmode == FRAME_BAR_NONE)
#define BAR_EXISTS(FRAME) ((FRAME)->barmode != FRAME_BAR_NONE)
#define BAR_H(FRAME) (FRAME)->bar_h

GR_DEFATTR(active);
GR_DEFATTR(inactive);
GR_DEFATTR(selected);
GR_DEFATTR(unselected);
GR_DEFATTR(tagged);
GR_DEFATTR(not_tagged);
GR_DEFATTR(dragged);
GR_DEFATTR(not_dragged);
GR_DEFATTR(activity);
GR_DEFATTR(no_activity);

static void ensure_create_attrs() {
  GR_ALLOCATTR_BEGIN;
  GR_ALLOCATTR(active);
  GR_ALLOCATTR(inactive);
  GR_ALLOCATTR(selected);
  GR_ALLOCATTR(unselected);
  GR_ALLOCATTR(tagged);
  GR_ALLOCATTR(not_tagged);
  GR_ALLOCATTR(dragged);
  GR_ALLOCATTR(not_dragged);
  GR_ALLOCATTR(no_activity);
  GR_ALLOCATTR(activity);
  GR_ALLOCATTR_END;
}

void frame_update_attr(WFrame *frame, int i, WRegion *reg) {
  GrStyleSpec *spec;
  bool selected, tagged, dragged, activity;
  if (i >= frame->titles_n) {
    return; /* Might happen when deinitialising */
  }
  ensure_create_attrs();
  spec = &frame->titles[i].attr;

  selected = (reg == FRAME_CURRENT(frame));
  tagged = (reg != NULL && reg->flags & REGION_TAGGED);
  dragged = (i == frame->tab_dragged_idx);
  activity = (reg != NULL && region_is_activity_r(reg));

  gr_stylespec_unalloc(spec);
  gr_stylespec_set(spec, selected ? GR_ATTR(selected) : GR_ATTR(unselected));
  gr_stylespec_set(spec, tagged ? GR_ATTR(tagged) : GR_ATTR(not_tagged));
  gr_stylespec_set(spec, dragged ? GR_ATTR(dragged) : GR_ATTR(not_dragged));
  gr_stylespec_set(spec, activity ? GR_ATTR(activity) : GR_ATTR(no_activity));
}

static uint get_spacing(const WFrame *frame) {
  GrBorderWidths bdw;
  if (frame->brush == NULL) return 0;
  grbrush_get_border_widths(frame->brush, &bdw);
  return bdw.spacing;
}

void frame_border_geom(const WFrame *frame, WRectangle *geom) {
  geom->x = 0; geom->y = 0;
  geom->w = REGION_GEOM(frame).w; geom->h = REGION_GEOM(frame).h;
  if (!BAR_INSIDE_BORDER(frame) && frame->brush != NULL) {
    geom->y += frame->bar_h;
    geom->h = maxof(0, geom->h - frame->bar_h);
  }
}

void frame_border_inner_geom(const WFrame *frame, WRectangle *geom) {
  GrBorderWidths bdw;
  frame_border_geom(frame, geom);
  if (frame->brush != NULL) {
    grbrush_get_border_widths(frame->brush, &bdw);
    geom->x += bdw.left; geom->y += bdw.top;
    geom->w = maxof(0, geom->w - (bdw.left + bdw.right));
    geom->h = maxof(0, geom->h - (bdw.top + bdw.bottom));
  }
}

void frame_bar_geom(const WFrame *frame, WRectangle *geom) {
  uint off;
  if (BAR_INSIDE_BORDER(frame)) {
    off = 0; /*get_spacing(frame);*/
    frame_border_inner_geom(frame, geom);
  } else {
    off = 0;
    geom->x = 0; geom->y = 0;
    geom->w = (frame->barmode == FRAME_BAR_SHAPED ? frame->bar_w : REGION_GEOM(frame).w);
  }
  geom->x += off; geom->y += off;
  geom->w = maxof(0, geom->w - 2 * off); geom->h = BAR_H(frame);
}

void frame_managed_geom(const WFrame *frame, WRectangle *geom) {
  uint spacing = get_spacing(frame);
  frame_border_inner_geom(frame, geom);
  if (BAR_INSIDE_BORDER(frame) && BAR_EXISTS(frame)) {
    geom->y += frame->bar_h + spacing;
    geom->h -= frame->bar_h + spacing;
  }
  geom->w = maxof(geom->w, 0); geom->h = maxof(geom->h, 0);
}

int frame_shaded_height(const WFrame *frame) {
  if (frame->barmode == FRAME_BAR_NONE) {
    return 0;
  } else if (!BAR_INSIDE_BORDER(frame)) {
    return frame->bar_h;
  } else {
    GrBorderWidths bdw;
    grbrush_get_border_widths(frame->brush, &bdw);
    return frame->bar_h + bdw.top + bdw.bottom;
  }
}

void frame_set_shape(WFrame *frame) {
  WRectangle gs[2];
  int n = 0;

  if (frame->brush != NULL) {
    if (BAR_EXISTS(frame)) {
      frame_bar_geom(frame, gs + n);
      n++;
    }
    frame_border_geom(frame, gs + n); n++;
    grbrush_set_window_shape(frame->brush, TRUE, n, gs);
  }
}

void frame_clear_shape(WFrame *frame) {
  if (frame->brush != NULL) grbrush_set_window_shape(frame->brush, TRUE, 0, NULL);
}

static void free_title(WFrame *frame, int i) {
  if (frame->titles[i].text != NULL) {
    free(frame->titles[i].text);
    frame->titles[i].text = NULL;
  }
}

void frame_draw(const WFrame *frame, bool complete) {
  WRectangle geom;
  if (frame->brush == NULL) return;

  frame_border_geom(frame, &geom);
  grbrush_begin(frame->brush, &geom, (complete ? 0 : GRBRUSH_NO_CLEAR_OK));
  grbrush_init_attr(frame->brush, &frame->baseattr);
  grbrush_draw_border(frame->brush, &geom);
  grbrush_end(frame->brush);
}

void frame_brushes_updated(WFrame *frame) {
  WFrameBarMode barmode;
  char *s;

  if (frame->brush == NULL) return;

  if (frame->mode == FRAME_MODE_FLOATING) {
    barmode = FRAME_BAR_SHAPED;
  } else if (frame->mode == FRAME_MODE_TILED || frame->mode == FRAME_MODE_UNKNOWN || frame->mode == FRAME_MODE_TRANSIENT_ALT) {
    barmode = FRAME_BAR_INSIDE;
  } else {
    barmode = FRAME_BAR_NONE;
  }

  if (grbrush_get_extra(frame->brush, "bar", 's', &s)) {
    if (strcmp(s, "inside") == 0) barmode = FRAME_BAR_INSIDE;
    else if (strcmp(s, "outside") == 0) barmode = FRAME_BAR_OUTSIDE;
    else if (strcmp(s, "shaped") == 0) barmode = FRAME_BAR_SHAPED;
    else if (strcmp(s, "none") == 0) barmode = FRAME_BAR_NONE;
    free(s);
  }

  frame->barmode = barmode;

  if (barmode == FRAME_BAR_NONE || frame->bar_brush == NULL) {
    frame->bar_h = 0;
  }
  frame_tabs_calc_brushes_updated(frame);
}

void frame_updategr(WFrame *frame) {
  frame_release_brushes(frame);
  frame_initialise_gr(frame);

  region_updategr_default((WRegion *)frame);

  mplex_fit_managed(&frame->mplex);
  window_draw((WWindow *)frame, TRUE);
}

void frame_initialise_gr(WFrame *frame) {
  Window win = frame->mplex.win.win;
  WRootWin *rw = region_rootwin_of((WRegion *)frame);
  frame->brush = gr_get_brush(win, rw, "frame");
  if (frame->brush == NULL) return;
  frame_brushes_updated(frame);
}

void frame_release_brushes(WFrame *frame) {
  if (frame->brush != NULL) { grbrush_release(frame->brush); frame->brush = NULL; }
}

void frame_setup_dragwin_style(WFrame *frame, GrStyleSpec *spec, int tab) {
  gr_stylespec_append(spec, &frame->baseattr);
  gr_stylespec_append(spec, &frame->titles[tab].attr);
}

void frame_inactivated(WFrame *frame) {
  ensure_create_attrs();
  gr_stylespec_set(&frame->baseattr, GR_ATTR(inactive));
  gr_stylespec_unset(&frame->baseattr, GR_ATTR(active));
  window_draw((WWindow *)frame, FALSE);
}

void frame_activated(WFrame *frame) {
  ensure_create_attrs();
  gr_stylespec_set(&frame->baseattr, GR_ATTR(active));
  gr_stylespec_unset(&frame->baseattr, GR_ATTR(inactive));
  window_draw((WWindow *)frame, FALSE);
}
