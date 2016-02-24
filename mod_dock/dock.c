#include <limits.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/extensions/shape.h>

#include <libtu/objp.h>
#include <libtu/map.h>
#include <libtu/minmax.h>
#include <libextl/extl.h>
#include <libextl/readconfig.h>
#include <libmainloop/defer.h>

#include <ioncore/common.h>
#include <ioncore/clientwin.h>
#include <ioncore/eventh.h>
#include <ioncore/global.h>
#include <ioncore/manage.h>
#include <ioncore/names.h>
#include <ioncore/property.h>
#include <ioncore/resize.h>
#include <ioncore/window.h>
#include <ioncore/mplex.h>
#include <ioncore/saveload.h>
#include <ioncore/bindmaps.h>
#include <ioncore/regbind.h>
#include <ioncore/extlconv.h>
#include <ioncore/event.h>
#include <ioncore/resize.h>
#include <ioncore/sizehint.h>
#include <ioncore/basicpholder.h>

#include "exports.h"
#include "../version.h"

static const char *modname = "dock";
const char mod_dock_ion_api_version[] = NOTION_API_VERSION;

static WBindmap *dock_bindmap = NULL;

INTRSTRUCT(WDockParam);
INTRSTRUCT(WDockApp);
INTRCLASS(WDock);

DECLSTRUCT(WDockParam) {
    const char *key;
    const char *desc;
    const StringIntMap *map;
    int dflt;
};

DECLSTRUCT(WDockApp) {
    WDockApp *next, *prev;
    WRegion *reg;
    int pos;
    bool draw_border;
    bool tile;
    WRectangle geom;
    WRectangle tile_geom;
    WRectangle border_geom;
};

DECLCLASS(WDock) {
    WWindow win;
    WDock *dock_next, *dock_prev;
    int pos, grow;
    bool is_auto;
    GrBrush *brush;
    WDockApp *dockapps;

    int min_w, min_h;
    int max_w, max_h;

    bool arrange_called;
    bool save;
};

static WDock *docks = NULL;

static void dock_param_extl_table_get(const WDockParam *param, ExtlTab conftab, int value) {
    const char *s;
    s = stringintmap_key(param->map, value, NULL);
    if (s) {
        extl_table_sets_s(conftab, param->key, s);
    }
}

static bool dock_param_do_set(const WDockParam *param, char *s, int *ret) {
    bool changed = FALSE;
    int i = stringintmap_value(param->map, s, -1);
    if (i < 0) {
        warn_obj(modname, "Invalid %s \"%s\"", param->desc, s);
    } else {
        if (*ret != i) {
            changed = TRUE;
        }
        *ret = i;
    }
    free(s);
    return changed;
}

static bool dock_param_extl_table_set(const WDockParam *param, ExtlTab conftab, int *ret) {
    char *s;
    if (extl_table_gets_s(conftab, param->key, &s))
        return dock_param_do_set(param, s, ret);
    return FALSE;
}

static bool dock_param_brush_set(const WDockParam *param, GrBrush *brush, int *ret) {
    char *s;
    if (grbrush_get_extra(brush, param->key, 's', &s))
        return dock_param_do_set(param, s, ret);
    return FALSE;
}

static const WDockParam dock_param_name = {"name", "name", NULL, 0};

#define DOCK_HPOS_MASK 0x000f
#define DOCK_HPOS_LEFT 0x0000
#define DOCK_VPOS_MASK 0x00f0
#define DOCK_VPOS_BOTTOM 0x0020

static StringIntMap dock_pos_map[] = {
    {"bl", DOCK_VPOS_BOTTOM | DOCK_HPOS_LEFT},
    END_STRINGINTMAP};

static WDockParam dock_param_pos = {"pos", "dock position", dock_pos_map, DOCK_HPOS_LEFT | DOCK_VPOS_BOTTOM};
enum WDockGrow { DOCK_GROW_RIGHT };
static StringIntMap dock_grow_map[] = { {"right", DOCK_GROW_RIGHT}, END_STRINGINTMAP};

WDockParam dock_param_grow = {"grow", "growth direction", dock_grow_map,
                              DOCK_GROW_RIGHT};

static const WDockParam dock_param_is_auto = {"is_auto", "is automatic", NULL,
                                              TRUE};

enum WDockOutlineStyle {
    DOCK_OUTLINE_STYLE_NONE,
    DOCK_OUTLINE_STYLE_ALL,
    DOCK_OUTLINE_STYLE_EACH
};

static StringIntMap dock_outline_style_map[] = {
    {"none", DOCK_OUTLINE_STYLE_NONE},
    {"all", DOCK_OUTLINE_STYLE_ALL},
    {"each", DOCK_OUTLINE_STYLE_EACH},
    END_STRINGINTMAP};

WDockParam dock_param_outline_style = {"outline_style", "outline style",
                                       dock_outline_style_map,
                                       DOCK_OUTLINE_STYLE_ALL};

static const WDockParam dock_param_tile_width = {"width", "tile width", NULL, 18};
static const WDockParam dock_param_tile_height = {"height", "tile height", NULL, 18};

/*}}}*/

/*{{{ Misc. */

#define CLIENTWIN_WINPROP_POSITION "dockposition"
#define CLIENTWIN_WINPROP_BORDER "dockborder"

static WDockApp *dock_find_dockapp(WDock *dock, WRegion *reg) {
    WDockApp *dockapp;
    for (dockapp = dock->dockapps; dockapp != NULL; dockapp = dockapp->next) {
        if (dockapp->reg == reg) {
            return dockapp;
        }
    }
    return NULL;
}

static void dock_get_outline_style(WDock *dock, int *ret) {
    *ret = dock_param_outline_style.dflt;
    if (dock->brush != NULL)
        dock_param_brush_set(&dock_param_outline_style, dock->brush, ret);
}

static void dock_get_tile_size(WDock *dock, WRectangle *ret) {
    ExtlTab tile_size_table;
    ret->x = 0;
    ret->y = 0;
    ret->w = dock_param_tile_width.dflt;
    ret->h = dock_param_tile_height.dflt;
    if (dock->brush == NULL) return;
    if (grbrush_get_extra(dock->brush, "tile_size", 't', &tile_size_table)) {
        extl_table_gets_i(tile_size_table, dock_param_tile_width.key, &ret->w);
        extl_table_gets_i(tile_size_table, dock_param_tile_height.key, &ret->h);
        extl_unref_table(tile_size_table);
    }
}

static void dock_get_pos_grow(WDock *dock, int *pos, int *grow) {
    *grow = dock->grow; *pos = dock->pos;
}

static void dock_reshape(WDock *dock) {
    int outline_style;

    if (!ioncore_g.shape_extension) return;

    dock_get_outline_style(dock, &outline_style);

    switch (outline_style) {
        case DOCK_OUTLINE_STYLE_NONE:
        case DOCK_OUTLINE_STYLE_EACH: {
            WDockApp *dockapp;

            /* Start with an empty set */
            XShapeCombineRectangles(ioncore_g.dpy, ((WWindow *)dock)->win,
                                    ShapeBounding, 0, 0, NULL, 0, ShapeSet, 0);

            /* Union with dockapp shapes */
            for (dockapp = dock->dockapps; dockapp != NULL;
                 dockapp = dockapp->next) {
                WClientWin *cwin = OBJ_CAST(dockapp->reg, WClientWin);
                if (outline_style == DOCK_OUTLINE_STYLE_EACH &&
                    dockapp->draw_border) {
                    /* Union with border shape */
                    XRectangle tile_rect;

                    tile_rect.x = dockapp->border_geom.x;
                    tile_rect.y = dockapp->border_geom.y;
                    tile_rect.width = dockapp->border_geom.w;
                    tile_rect.height = dockapp->border_geom.h;
                    XShapeCombineRectangles(
                        ioncore_g.dpy, ((WWindow *)dock)->win, ShapeBounding, 0,
                        0, &tile_rect, 1, ShapeUnion, 0);
                } else if (cwin != NULL) {
                    /* Union with dockapp shape */
                    int count;
                    int ordering;

                    XRectangle *rects =
                        XShapeGetRectangles(ioncore_g.dpy, cwin->win,
                                            ShapeBounding, &count, &ordering);
                    if (rects != NULL) {
                        WRectangle dockapp_geom = REGION_GEOM(cwin);
                        XShapeCombineRectangles(
                            ioncore_g.dpy, ((WWindow *)dock)->win,
                            ShapeBounding, dockapp_geom.x, dockapp_geom.y,
                            rects, count, ShapeUnion, ordering);
                        XFree(rects);
                    }
                }
            }
        } break;

        case DOCK_OUTLINE_STYLE_ALL: {
            WRectangle geom;
            XRectangle rect;
            geom = REGION_GEOM(dock);
            rect.x = 0; rect.y = 0;
            rect.width = geom.w; rect.height = geom.h;
            XShapeCombineRectangles(ioncore_g.dpy, ((WWindow *)dock)->win,
                                    ShapeBounding, 0, 0, &rect, 1, ShapeSet, 0);
        } break;
    }
}

static void dock_arrange_dockapps(WDock *dock, const WRectangle *bd_dockg,
                                  const WDockApp *replace_this,
                                  WDockApp *with_this) {
    GrBorderWidths dock_bdw, dockapp_bdw;
    WDockApp dummy_copy, *dockapp;
    int pos, grow, cur_coord = 0;
    WRectangle dock_geom;

    dock->arrange_called = TRUE;

    dock_get_pos_grow(dock, &pos, &grow);

    /* Determine dock and dockapp border widths */
    memset(&dock_bdw, 0, sizeof(GrBorderWidths));
    memset(&dockapp_bdw, 0, sizeof(GrBorderWidths));

    if (dock->brush) {
        int outline_style;

        dock_get_outline_style(dock, &outline_style);
        switch (outline_style) {
            case DOCK_OUTLINE_STYLE_NONE:
                break;
            case DOCK_OUTLINE_STYLE_ALL:
                grbrush_get_border_widths(dock->brush, &dock_bdw);
                dockapp_bdw.spacing = dock_bdw.spacing;
                break;
            case DOCK_OUTLINE_STYLE_EACH:
                grbrush_get_border_widths(dock->brush, &dockapp_bdw);
                break;
        }
    }
    dock_geom.w = bd_dockg->w - dock_bdw.left - dock_bdw.right;
    dock_geom.h = bd_dockg->h - dock_bdw.top - dock_bdw.bottom;
    /* Calculate initial co-ordinate for layout algorithm */
    switch (grow) {
        case DOCK_GROW_RIGHT:
            cur_coord = dock_bdw.left;
            break;
    }

    /* Arrange dockapps */
    for (dockapp = dock->dockapps; dockapp != NULL; dockapp = dockapp->next) {
        WDockApp *da = dockapp;

        if (replace_this != NULL) {
            if (replace_this == dockapp) {
                da = with_this;
            } else {
                dummy_copy = *dockapp;
                da = &dummy_copy;
            }
        }

        da->border_geom.y = dock_geom.h - da->border_geom.h;
        da->border_geom.y += dock_bdw.top;
        da->border_geom.x = cur_coord;
        cur_coord += da->border_geom.w + dockapp_bdw.spacing;

        /* Calculate tile geom */
        da->tile_geom.x = da->border_geom.x + dockapp_bdw.left;
        da->tile_geom.y = da->border_geom.y + dockapp_bdw.top;

        /* Calculate dockapp geom */
        if (da->tile) {
            da->geom.x = da->tile_geom.x + (da->tile_geom.w - da->geom.w) / 2;
            da->geom.y = da->tile_geom.y + (da->tile_geom.h - da->geom.h) / 2;
        } else {
            da->geom.x = da->tile_geom.x; da->geom.y = da->tile_geom.y;
        }

        if (replace_this == NULL)
            region_fit(da->reg, &(da->geom), REGION_FIT_BOUNDS);
    }
}

static void dock_set_minmax(WDock *dock, int grow, const WRectangle *g) {
    dock->min_w = g->w; dock->min_h = g->h;
    dock->max_w = INT_MAX; dock->max_h = g->h;
}

static void dockapp_calc_preferred_size(WDock *dock, int grow,
                                        const WRectangle *tile_size,
                                        WDockApp *da) {
    int w = da->geom.w, h = da->geom.h;
    da->geom.w = w; da->geom.h = minof(h, tile_size->h);
    region_size_hints_correct(da->reg, &(da->geom.w), &(da->geom.h), TRUE);
}

static void dock_managed_rqgeom_(WDock *dock, WRegion *reg, int flags,
                                 const WRectangle *geom, WRectangle *geomret,
                                 bool just_update_minmax) {
    WDockApp *dockapp = NULL, *thisdockapp = NULL, thisdockapp_copy;
    WRectangle dock_geom, border_dock_geom;
    GrBorderWidths dock_bdw, dockapp_bdw;
    int n_dockapps = 0, max_w = 1, max_h = 1, total_w = 0, total_h = 0;
    int pos, grow;
    WRectangle tile_size;

    /* dock_resize calls with NULL parameters. */
    assert(reg != NULL ||
           (geomret == NULL && !(flags & REGION_RQGEOM_TRYONLY)));

    dock_get_pos_grow(dock, &pos, &grow);
    dock_get_tile_size(dock, &tile_size);

    /* Determine dock and dockapp border widths */
    memset(&dock_bdw, 0, sizeof(GrBorderWidths));
    memset(&dockapp_bdw, 0, sizeof(GrBorderWidths));

    if (dock->brush) {
        int outline_style;

        dock_get_outline_style(dock, &outline_style);
        switch (outline_style) {
            case DOCK_OUTLINE_STYLE_NONE:
                break;
            case DOCK_OUTLINE_STYLE_ALL:
                grbrush_get_border_widths(dock->brush, &dock_bdw);
                dockapp_bdw.spacing = dock_bdw.spacing;
                break;
            case DOCK_OUTLINE_STYLE_EACH:
                grbrush_get_border_widths(dock->brush, &dockapp_bdw);
                break;
        }
    }

    /* Calculate widths and heights */
    for (dockapp = dock->dockapps; dockapp != NULL; dockapp = dockapp->next) {
        WDockApp *da = dockapp;
        bool update = !(flags & REGION_RQGEOM_TRYONLY);
        if (dockapp->reg == reg) {
            thisdockapp = dockapp;
            if (flags & REGION_RQGEOM_TRYONLY) {
                thisdockapp_copy = *dockapp;
                thisdockapp_copy.geom = *geom;
                da = &thisdockapp_copy;
                update = TRUE;
            }
            da->geom = *geom;
        }

        if (update) {
            /* Calculcate preferred size */
            dockapp_calc_preferred_size(dock, grow, &tile_size, da);

            /* Determine whether dockapp should be placed on a tile */
            da->tile = da->geom.w <= tile_size.w && da->geom.h <= tile_size.h;

            /* Calculate width and height */
            if (da->tile) {
                da->tile_geom.w = tile_size.w; da->tile_geom.h = tile_size.h;
            } else {
                da->tile_geom.w = da->geom.w; da->tile_geom.h = da->geom.h;
            }

            /* Calculate border width and height */
            da->border_geom.w = dockapp_bdw.left + da->tile_geom.w + dockapp_bdw.right;
            da->border_geom.h = dockapp_bdw.top + da->tile_geom.h + dockapp_bdw.right;
        }

        /* Calculate maximum and accumulated widths and heights */
        if (da->border_geom.w > max_w) max_w = da->border_geom.w;
        total_w += da->border_geom.w + (n_dockapps ? dockapp_bdw.spacing : 0);
        if (da->border_geom.h > max_h) max_h = da->border_geom.h;
        total_h += da->border_geom.h + (n_dockapps ? dockapp_bdw.spacing : 0);
        /* Count dockapps */
        ++n_dockapps;
    }

    if (thisdockapp == NULL && reg != NULL) {
        warn("Requesting dockapp not found.");
        if (geomret) *geomret = REGION_GEOM(reg);
        return;
    }

    /* Calculate width and height of dock */
    if (n_dockapps) {
        dock_geom.w = total_w;
        dock_geom.h = max_h;
    }

    border_dock_geom.x = REGION_GEOM(dock).x;
    border_dock_geom.y = REGION_GEOM(dock).y;
    border_dock_geom.w = dock_bdw.left + dock_geom.w + dock_bdw.right;
    border_dock_geom.h = dock_bdw.top + dock_geom.h + dock_bdw.bottom;

    /* Fit dock to new geom if required */
    if (!(flags & REGION_RQGEOM_TRYONLY)) {
        WRQGeomParams rq = RQGEOMPARAMS_INIT;
        dock_set_minmax(dock, grow, &border_dock_geom);
        if (just_update_minmax) return;
        rq.flags = REGION_RQGEOM_WEAK_X | REGION_RQGEOM_WEAK_Y;
        rq.geom = border_dock_geom;
        dock->arrange_called = FALSE;
        region_rqgeom((WRegion *)dock, &rq, NULL);
        if (!dock->arrange_called)
            dock_arrange_dockapps(dock, &REGION_GEOM(dock), NULL, NULL);
        if (thisdockapp != NULL && geomret != NULL)
            *geomret = thisdockapp->geom;
    } else {
        if (thisdockapp != NULL && geomret != NULL) {
            dock_arrange_dockapps(dock, &REGION_GEOM(dock), thisdockapp,
                                  &thisdockapp_copy);
            *geomret = thisdockapp_copy.geom;
        }
    }
}

static void dock_managed_rqgeom(WDock *dock, WRegion *reg,
                                const WRQGeomParams *rq, WRectangle *geomret) {
    dock_managed_rqgeom_(dock, reg, rq->flags, &rq->geom, geomret, FALSE);
}

void dock_size_hints(WDock *dock, WSizeHints *hints) {
    hints->min_set = TRUE;
    hints->min_width = dock->min_w; hints->min_height = dock->min_h;
    hints->max_set = TRUE;
    hints->max_width = dock->max_w; hints->max_height = dock->max_h;
}

static bool dock_fitrep(WDock *dock, WWindow *parent, const WFitParams *fp) {
    if (!window_fitrep(&(dock->win), parent, fp)) return FALSE;
    dock_arrange_dockapps(dock, &(fp->g), NULL, NULL);
    if (ioncore_g.shape_extension) dock_reshape(dock);
    return TRUE;
}

static int dock_orientation(WDock *dock) {
    return REGION_ORIENTATION_HORIZONTAL;
}

static void dock_draw(WDock *dock, bool complete) {
    int outline_style;
    WRectangle g;

    if (dock->brush == NULL) return;

    g.x = 0; g.y = 0;
    g.w = REGION_GEOM(dock).w; g.h = REGION_GEOM(dock).h;

    grbrush_begin(dock->brush, &g, (complete ? 0 : GRBRUSH_NO_CLEAR_OK));

    dock_get_outline_style(dock, &outline_style);
    switch (outline_style) {
        case DOCK_OUTLINE_STYLE_NONE:
            break;
        case DOCK_OUTLINE_STYLE_ALL: {
            WRectangle geom = REGION_GEOM(dock);
            geom.x = geom.y = 0;
            grbrush_draw_border(dock->brush, &geom);
        } break;
        case DOCK_OUTLINE_STYLE_EACH: {
            WDockApp *dockapp;
            for (dockapp = dock->dockapps; dockapp != NULL;
                 dockapp = dockapp->next) {
                grbrush_draw_border(dock->brush, &dockapp->tile_geom);
            }
        } break;
    }

    grbrush_end(dock->brush);
}

/*EXTL_DOC
 * Resizes and refreshes \var{dock}.
 */
EXTL_EXPORT_MEMBER
void dock_resize(WDock *dock) {
    dock_managed_rqgeom_(dock, NULL, 0, NULL, NULL, FALSE);
    dock_draw(dock, TRUE);
}

static void dock_brush_release(WDock *dock) {
    if (dock->brush) {
        grbrush_release(dock->brush);
        dock->brush = NULL;
    }
}

static void dock_brush_get(WDock *dock) {
    dock_brush_release(dock);
    dock->brush =
        gr_get_brush(((WWindow *)dock)->win, region_rootwin_of((WRegion *)dock),
                     "stdisp-dock");
}

static void dock_updategr(WDock *dock) {
    dock_brush_get(dock);
    dock_resize(dock);
}

static void mplexpos(int pos, int *mpos) {
    int hp = pos & DOCK_HPOS_MASK, vp = pos & DOCK_VPOS_MASK;
    *mpos = MPLEX_STDISP_BL;
}

static void mplexszplcy(int pos, WSizePolicy *szplcy) {
    int hp = pos & DOCK_HPOS_MASK, vp = pos & DOCK_VPOS_MASK;
    WSizePolicy p = SIZEPOLICY_GRAVITY_SOUTHWEST;
    *szplcy = p;
}

static void dock_do_set(WDock *dock, ExtlTab conftab, bool resize) {
    char *s;
    bool b; bool save = FALSE;
    if (extl_table_gets_s(conftab, dock_param_name.key, &s)) {
        if (!region_set_name((WRegion *)dock, s)) {
            warn_obj(modname, "Can't set name to \"%s\"", s);
        }
        free(s);
    }

    if (extl_table_gets_b(conftab, "save", &save)) dock->save = save;
    if (extl_table_gets_b(conftab, dock_param_is_auto.key, &b))
        dock->is_auto = b;
    if (resize) {
        WMPlex *par = OBJ_CAST(REGION_PARENT(dock), WMPlex);
        dock_resize(dock);
    }
}

/*EXTL_DOC
 * Configure \var{dock}. \var{conftab} is a table of key/value pairs:
 *
 * \begin{tabularx}{\linewidth}{llX}
 *  \tabhead{Key & Values & Description}
 *  \var{name} & string & Name of dock \\
 *  \var{pos} & string in $\{t,m,b\}\times\{t,c,b\}$ & Dock position.
 *       Can only be used in floating mode. \\
 *  \var{grow} & up/down/left/right &
 *       Growth direction where new dockapps are added. Also
 *       sets orientation for dock when working as WMPlex status
 *       display (see \fnref{WMPlex.set_stdisp}). \\
 *  \var{is_auto} & bool &
 *       Should \var{dock} automatically manage new dockapps? \\
 * \end{tabularx}
 *
 * Any parameters not explicitly set in \var{conftab} will be left unchanged.
 */
EXTL_EXPORT_MEMBER
void dock_set(WDock *dock, ExtlTab conftab) {
    dock_do_set(dock, conftab, TRUE);
}

static void dock_do_get(WDock *dock, ExtlTab conftab) {
    extl_table_sets_s(conftab, dock_param_name.key, region_name((WRegion *)dock));
    dock_param_extl_table_get(&dock_param_pos, conftab, dock->pos);
    dock_param_extl_table_get(&dock_param_grow, conftab, dock->grow);
    extl_table_sets_b(conftab, dock_param_is_auto.key, dock->is_auto);
    extl_table_sets_b(conftab, "save", dock->save);
}

/*EXTL_DOC
 * Get \var{dock}'s configuration table. See \fnref{WDock.set} for a
 * description of the table.
 */
EXTL_SAFE
EXTL_EXPORT_MEMBER
ExtlTab dock_get(WDock *dock) {
    ExtlTab conftab;

    conftab = extl_create_table();
    dock_do_get(dock, conftab);
    return conftab;
}

static bool dock_init(WDock *dock, WWindow *parent, const WFitParams *fp) {
    WFitParams fp2 = *fp;

    dock->pos = dock_param_pos.dflt;
    dock->grow = dock_param_grow.dflt;
    dock->is_auto = dock_param_is_auto.dflt;
    dock->brush = NULL;
    dock->dockapps = NULL;
    dock->min_w = 1; dock->min_h = 1;
    dock->max_w = 1; dock->max_h = 1;
    dock->arrange_called = FALSE;
    dock->save = TRUE;

    if (!window_init((WWindow *)dock, parent, &fp2, "WDock")) return FALSE;
    region_add_bindmap((WRegion *)dock, dock_bindmap);
    window_select_input(&(dock->win), IONCORE_EVENTMASK_CWINMGR);
    dock_brush_get(dock);
    LINK_ITEM(docks, dock, dock_next, dock_prev);

    return TRUE;
}

static WDock *create_dock(WWindow *parent, const WFitParams *fp) {
    CREATEOBJ_IMPL(WDock, dock, (p, parent, fp));
}

static void dock_deinit(WDock *dock) {
    while (dock->dockapps != NULL) destroy_obj((Obj *)dock->dockapps->reg);
    UNLINK_ITEM(docks, dock, dock_next, dock_prev);
    dock_brush_release(dock);
    window_deinit((WWindow *)dock);
}

EXTL_EXPORT
WDock *mod_dock_create(ExtlTab tab) {
    char *mode            = NULL;
    bool floating         = FALSE;
    int screenid          = 0;
    WScreen *screen       = NULL;
    WDock *dock           = NULL;
    WRegion *stdisp       = NULL;
    WMPlexSTDispInfo din;
    WFitParams fp;

    extl_table_gets_i(tab, "screen", &screenid);
    screen = ioncore_find_screen_id(screenid);

    /* Create the dock */
    fp.mode = REGION_FIT_BOUNDS | REGION_FIT_WHATEVER;
    fp.g.x = 0; fp.g.y = 0;
    fp.g.w = 1; fp.g.h = 1;
    dock = create_dock((WWindow *)screen, &fp);
    if (dock == NULL) {
        warn("Failed to create dock.");
        return NULL;
    }
    dock->save = FALSE; dock_do_set(dock, tab, FALSE);

    /* Calculate min/max size */
    dock_managed_rqgeom_(dock, NULL, 0, NULL, NULL, TRUE);
    mplexpos(dock->pos, &din.pos);
    din.fullsize = FALSE; /* not supported */
    if (mplex_set_stdisp((WMPlex *)screen, (WRegion *)dock, &din))
        return dock;

    /* Failed to attach. */
    warn("Failed to attach dock to screen.");
    destroy_obj((Obj *)dock);
    return NULL;
}

/*EXTL_DOC
 * Toggle floating docks on \var{mplex}.
 */
EXTL_EXPORT
void mod_dock_set_floating_shown_on(WMPlex *mplex, const char *how) {
    int setpar = libtu_setparam_invert(libtu_string_to_setparam(how));
    WDock *dock;
    for (dock = docks; dock; dock = dock->dock_next) {
        if (REGION_MANAGER(dock) == (WRegion *)mplex)
            mplex_set_hidden(mplex, (WRegion *)dock, setpar);
    }
}

ExtlTab dock_get_configuration(WDock *dock) {
    ExtlTab tab;
    if (dock->save == FALSE) return extl_table_none();
    tab = region_get_base_configuration((WRegion *)dock);
    dock_do_get(dock, tab);
    return tab;
}

WRegion *dock_load(WWindow *par, const WFitParams *fp, ExtlTab tab) {
    WDock *dock = create_dock(par, fp);
    if (dock != NULL) { dock_set(dock, tab); dock_fitrep(dock, NULL, fp); }
    return (WRegion *)dock;
}

static bool dock_do_attach_final(WDock *dock, WRegion *reg, void *UNUSED(unused)) {
    WDockApp *dockapp, *before_dockapp;
    WRectangle geom;
    bool draw_border = TRUE;
    int pos = INT_MAX;
    /* Create and initialise a new WDockApp struct */
    dockapp = ALLOC(WDockApp);
    if (dockapp == NULL) return FALSE;
    if (OBJ_IS(reg, WClientWin)) {
        ExtlTab proptab = ((WClientWin *)reg)->proptab;
        extl_table_gets_b(proptab, CLIENTWIN_WINPROP_BORDER, &draw_border);
        extl_table_gets_i(proptab, CLIENTWIN_WINPROP_POSITION, &pos);
    }
    dockapp->reg = reg; dockapp->draw_border = draw_border;
    dockapp->pos = pos; dockapp->tile = FALSE;
    /* Insert the dockapp at the correct relative position */
    before_dockapp = dock->dockapps;
    for (before_dockapp = dock->dockapps;
         before_dockapp != NULL && dockapp->pos >= before_dockapp->pos;
         before_dockapp = before_dockapp->next) {
    }
    if (before_dockapp != NULL) {
        LINK_ITEM_BEFORE(dock->dockapps, before_dockapp, dockapp, next, prev);
    } else {
        LINK_ITEM(dock->dockapps, dockapp, next, prev);
    }
    region_set_manager(reg, (WRegion *)dock);
    geom = REGION_GEOM(reg);
    dock_managed_rqgeom_(dock, reg, REGION_RQGEOM_WEAK_X | REGION_RQGEOM_WEAK_Y, &geom, NULL, FALSE);
    region_map(reg);
    return TRUE;
}

static WRegion *dock_do_attach(WDock *dock, WRegionAttachData *data) {
    WFitParams fp;
    dock_get_tile_size(dock, &(fp.g));
    fp.g.x = 0; fp.g.y = 0;
    fp.mode = REGION_FIT_WHATEVER | REGION_FIT_BOUNDS;
    return region_attach_helper((WRegion *)dock, (WWindow *)dock, &fp,
                                (WRegionDoAttachFn *)dock_do_attach_final, NULL,
                                data);
}

/*EXTL_DOC
 * Attach \var{reg} to \var{dock}.
 */
EXTL_EXPORT_MEMBER
bool dock_attach(WDock *dock, WRegion *reg) {
    WRegionAttachData data;
    data.type = REGION_ATTACH_REPARENT;
    data.u.reg = reg;
    return (dock_do_attach(dock, &data) != NULL);
}

static bool dock_handle_drop(WDock *dock, int UNUSED(x), int UNUSED(y),
                             WRegion *dropped) {
    return dock_attach(dock, dropped);
}

static WRegion *dock_ph_handler(WDock *dock, int UNUSED(flags), WRegionAttachData *data) {
    return dock_do_attach(dock, data);
}

static WPHolder *dock_managed_get_pholder(WDock *dock, WRegion *UNUSED(mgd)) {
    return (WPHolder *)create_basicpholder(
        (WRegion *)dock, ((WBasicPHolderHandler *)dock_ph_handler));
}

static WPHolder *dock_prepare_manage(WDock *dock,
                                     const WClientWin *UNUSED(cwin),
                                     const WManageParams *UNUSED(param),
                                     int priority) {
    if (!MANAGE_PRIORITY_OK(priority, MANAGE_PRIORITY_LOW)) return NULL;
    return (WPHolder *)create_basicpholder( (WRegion *)dock, ((WBasicPHolderHandler *)dock_ph_handler));
}

static void dock_managed_remove(WDock *dock, WRegion *reg) {
    WDockApp *dockapp = dock_find_dockapp(dock, reg);
    if (dockapp == NULL) return;
    UNLINK_ITEM(dock->dockapps, dockapp, next, prev);
    free(dockapp);
    region_unset_manager(reg, (WRegion *)dock);
    dock_resize(dock);
}

static bool dock_clientwin_is_dockapp(WClientWin *cwin, const WManageParams *param) {
    bool is_dockapp = FALSE;

    /* First, inspect the WManageParams.dockapp parameter */
    if (param->dockapp) {
        is_dockapp = TRUE;
    }

    /* Second, inspect the _NET_WM_WINDOW_TYPE property */
    if (!is_dockapp) {
        static Atom atom__net_wm_window_type = None;
        static Atom atom__net_wm_window_type_dock = None;
        Atom actual_type = None;
        int actual_format;
        unsigned long nitems;
        unsigned long bytes_after;
        unsigned char *prop;

        if (atom__net_wm_window_type == None) {
            atom__net_wm_window_type =
                XInternAtom(ioncore_g.dpy, "_NET_WM_WINDOW_TYPE", False);
        }
        if (atom__net_wm_window_type_dock == None) {
            atom__net_wm_window_type_dock =
                XInternAtom(ioncore_g.dpy, "_NET_WM_WINDOW_TYPE_DOCK", False);
        }
        if (XGetWindowProperty(ioncore_g.dpy, cwin->win,
                               atom__net_wm_window_type, 0, sizeof(Atom), False,
                               XA_ATOM, &actual_type, &actual_format, &nitems,
                               &bytes_after, &prop) == Success) {
            if (actual_type == XA_ATOM && nitems >= 1 &&
                *(Atom *)prop == atom__net_wm_window_type_dock) {
                is_dockapp = TRUE;
            }
            XFree(prop);
        }
    }

    /* Third, inspect the WM_CLASS property */
    if (!is_dockapp) {
        char **p;
        int n;
        p = xwindow_get_text_property(cwin->win, XA_WM_CLASS, &n);
        if (p != NULL) {
            if (n >= 2 && strcmp(p[1], "DockApp") == 0) {
                is_dockapp = TRUE;
            }
            XFreeStringList(p);
        }
    }

    /* Fourth, inspect the _KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR property */
    if (!is_dockapp) {
        static Atom atom__kde_net_wm_system_tray_window_for = None;
        Atom actual_type = None;
        int actual_format;
        unsigned long nitems;
        unsigned long bytes_after;
        unsigned char *prop;

        if (atom__kde_net_wm_system_tray_window_for == None) {
            atom__kde_net_wm_system_tray_window_for = XInternAtom(
                ioncore_g.dpy, "_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR", False);
        }
        if (XGetWindowProperty(ioncore_g.dpy, cwin->win,
                               atom__kde_net_wm_system_tray_window_for, 0,
                               sizeof(Atom), False, AnyPropertyType,
                               &actual_type, &actual_format, &nitems,
                               &bytes_after, &prop) == Success) {
            if (actual_type != None) {
                is_dockapp = TRUE;
            }
            XFree(prop);
        }
    }

    return is_dockapp;
}

static WDock *dock_find_suitable_dock(WClientWin *cwin, const WManageParams *param) {
    WDock *dock;
    for (dock = docks; dock; dock = dock->dock_next) {
        if (!dock->is_auto) continue;
        if (!region_same_rootwin((WRegion *)dock, (WRegion *)cwin)) continue;
        break;
    }
    return dock;
}

static bool clientwin_do_manage_hook(WClientWin *cwin, const WManageParams *param) {
    WDock *dock;
    if (!dock_clientwin_is_dockapp(cwin, param)) {
        return FALSE;
    }
    dock = dock_find_suitable_dock(cwin, param);
    if (!dock) { return FALSE; }
    return region_manage_clientwin((WRegion *)dock, cwin, param, MANAGE_PRIORITY_NONE);
}

bool mod_dock_init() {
    if (!ioncore_register_regclass(&CLASSDESCR(WDock), (WRegionLoadCreateFn *)dock_load)) {
        return FALSE;
    }
    if (!mod_dock_register_exports()) {
        ioncore_unregister_regclass(&CLASSDESCR(WDock));
        return FALSE;
    }
    dock_bindmap = ioncore_alloc_bindmap("WDock", NULL);
    if (dock_bindmap == NULL) {
        warn("Unable to allocate dock bindmap.");
        mod_dock_unregister_exports();
        ioncore_unregister_regclass(&CLASSDESCR(WDock));
    }
    extl_read_config("cfg_dock", NULL, TRUE);
    hook_add(clientwin_do_manage_alt, (WHookDummy *)clientwin_do_manage_hook);
    return TRUE;
}

void mod_dock_deinit() {
    WDock *dock;
    ioncore_unregister_regclass(&CLASSDESCR(WDock));
    hook_remove(clientwin_do_manage_alt, (WHookDummy *)clientwin_do_manage_hook);
    dock = docks;
    while (dock != NULL) {
        WDock *next = dock->dock_next;
        destroy_obj((Obj *)dock);
        dock = next;
    }
    mod_dock_unregister_exports();
    if (dock_bindmap != NULL) {
        ioncore_free_bindmap("WDock", dock_bindmap);
        dock_bindmap = NULL;
    }
}

static DynFunTab dock_dynfuntab[] = {
    {window_draw, dock_draw},
    {region_updategr, dock_updategr},
    {region_managed_rqgeom, dock_managed_rqgeom},
    {(DynFun *)region_prepare_manage, (DynFun *)dock_prepare_manage},
    {region_managed_remove, dock_managed_remove},
    {(DynFun *)region_get_configuration, (DynFun *)dock_get_configuration},
    {region_size_hints, dock_size_hints},
    {(DynFun *)region_fitrep, (DynFun *)dock_fitrep},
    {(DynFun *)region_orientation, (DynFun *)dock_orientation},
    {(DynFun *)region_handle_drop, (DynFun *)dock_handle_drop},
    {(DynFun *)region_managed_get_pholder, (DynFun *)dock_managed_get_pholder},
    END_DYNFUNTAB};

EXTL_EXPORT
IMPLCLASS(WDock, WWindow, dock_deinit, dock_dynfuntab);
