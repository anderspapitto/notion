TOPDIR=../..
include $(TOPDIR)/build/system-inc.mk

INCLUDES += $(X11_INCLUDES) -I$(TOPDIR)
LIBS += $(X11_LIBS) 
EXTRA_EXECUTABLE = notionflux
SOURCES=notionflux.c

include $(TOPDIR)/build/rules.mk

_install: executable_install
