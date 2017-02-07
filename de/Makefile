TOPDIR=..
include $(TOPDIR)/build/system-inc.mk

INCLUDES += $(X11_INCLUDES) $(LIBTU_INCLUDES) $(LIBEXTL_INCLUDES) -I..
CFLAGS += $(C99_SOURCE)

SOURCES=init.c draw.c font.c colour.c brush.c fontset.c style.c precompose.c
MODULE=de
MAKE_EXPORTS=de

include $(TOPDIR)/build/rules.mk

init.o: exports.h

_install: module_install
