TOPDIR=..
include $(TOPDIR)/build/system-inc.mk

INCLUDES += $(X11_INCLUDES) $(LIBTU_INCLUDES) $(LIBEXTL_INCLUDES) -I..
CFLAGS += $(C99_SOURCE)

SOURCES=dock.c
DOCS=LICENSE

MAKE_EXPORTS=mod_dock

MODULE=mod_dock

include $(TOPDIR)/build/rules.mk

_install: module_install

dock.o: exports.h

.PHONY: tags
tags:
	exuberant-ctags -R . $(TOPDIR)
