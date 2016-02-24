#include <libtu/objp.h>
#include <libtu/obj.h>
#include <libtu/pointer.h>
#include "basicpholder.h"

bool basicpholder_init(WBasicPHolder *ph, WRegion *reg, WBasicPHolderHandler *hnd) {
    assert(reg != NULL && hnd != NULL);
    pholder_init(&(ph->ph));
    watch_init(&(ph->reg_watch));
    if (!watch_setup(&(ph->reg_watch), (Obj *)reg, NULL)) {
        pholder_deinit(&(ph->ph));
        return FALSE;
    }
    ph->hnd = hnd;
    return TRUE;
}

WBasicPHolder *create_basicpholder(WRegion *reg, WBasicPHolderHandler *hnd) {
    CREATEOBJ_IMPL(WBasicPHolder, basicpholder, (p, reg, hnd));
}

void basicpholder_deinit(WBasicPHolder *ph) {
    watch_reset(&(ph->reg_watch));
    pholder_deinit(&(ph->ph));
}

WRegion *basicpholder_do_attach(WBasicPHolder *ph, int flags, WRegionAttachData *data) {
    WRegion *reg = (WRegion *)ph->reg_watch.obj;
    if (reg == NULL || ph->hnd == NULL) return FALSE;
    return ph->hnd(reg, flags, data);
}

bool basicpholder_do_goto(WBasicPHolder *ph) {
    WRegion *reg = (WRegion *)ph->reg_watch.obj;
    if (reg != NULL) return region_goto((WRegion *)reg);
    return FALSE;
}

WRegion *basicpholder_do_target(WBasicPHolder *ph) {
    return (WRegion *)ph->reg_watch.obj;
}

static DynFunTab basicpholder_dynfuntab[] = {
    {(DynFun *)pholder_do_attach, (DynFun *)basicpholder_do_attach},
    {(DynFun *)pholder_do_goto, (DynFun *)basicpholder_do_goto},
    {(DynFun *)pholder_do_target, (DynFun *)basicpholder_do_target},
    END_DYNFUNTAB};
IMPLCLASS(WBasicPHolder, WPHolder, basicpholder_deinit, basicpholder_dynfuntab);
