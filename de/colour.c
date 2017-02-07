#include <ioncore/common.h>
#include "colour.h"

bool de_alloc_colour(WRootWin *rootwin, DEColour *ret, const char *name) {
    if(name==NULL)
        return FALSE;
    return XftColorAllocName(
        ioncore_g.dpy,
        XftDEDefaultVisual(),
        rootwin->default_cmap,
        name,
        ret);
}

bool de_duplicate_colour(WRootWin *rootwin, DEColour in, DEColour *out) {
    return XftColorAllocValue(
        ioncore_g.dpy,
        XftDEDefaultVisual(),
        rootwin->default_cmap,
        &(in.color),
        out);
}

void de_free_colour_group(WRootWin *rootwin, DEColourGroup *cg) {
    de_free_colour(rootwin, cg->bg);
    de_free_colour(rootwin, cg->fg);
    de_free_colour(rootwin, cg->hl);
    de_free_colour(rootwin, cg->sh);
    de_free_colour(rootwin, cg->pad);
}

void de_free_colour(WRootWin *rootwin, DEColour col) {
    XftColorFree(ioncore_g.dpy, XftDEDefaultVisual(), rootwin->default_cmap, &col);
}
