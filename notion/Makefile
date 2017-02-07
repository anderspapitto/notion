TOPDIR=..
include $(TOPDIR)/build/system-inc.mk

MODULE_LIST = mod_tiling mod_dock mod_sm de mod_xrandr mod_notionflux ioncore
EXECUTABLE = notion
SOURCES = notion.c

INCLUDES += $(X11_INCLUDES)
INCLUDES += $(LIBMAINLOOP_INCLUDES) $(LIBTU_INCLUDES) $(LIBEXTL_INCLUDES)

LIBS += $(X11_LIBS) -lSM -lICE
LIBS += $(WHOLEA) $(LIBMAINLOOP_LIBS) $(LIBEXTL_LIBS) $(LIBTU_LIBS) $(NO_WHOLEA)
LIBS += $(LUA_LIBS) $(DL_LIBS)
LIBS += -lm

MODULE_PATH = $(TOPDIR)

EXT_OBJS += ../ioncore/ioncore.a

DEFINES += -DETCDIR=\"$(ETCDIR)\" -DSHAREDIR=\"$(SHAREDIR)\" \
           -DEXTRABINDIR=\"$(EXTRABINDIR)\" -DMODULEDIR=\"$(MODULEDIR)\" \
	       -DLCDIR=\"$(LCDIR)\" -DLOCALEDIR=\"$(LOCALEDIR)\"

CFLAGS += $(C99_SOURCE)

include $(TOPDIR)/build/rules.mk

_install: executable_install
