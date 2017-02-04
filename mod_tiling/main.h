#ifndef ION_MOD_TILING_MAIN_H
#define ION_MOD_TILING_MAIN_H

#include <ioncore/binding.h>
#include <ioncore/regbind.h>

extern bool mod_tiling_init();
extern void mod_tiling_deinit();

extern WBindmap *mod_tiling_tiling_bindmap;
extern WBindmap *mod_tiling_frame_bindmap;

extern int mod_tiling_raise_delay;

#endif
