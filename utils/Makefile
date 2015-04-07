##
## Notion utils/ Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/build/system-inc.mk

######################################

SUBDIRS=
INSTALL_SUBDIRS=$(SUBDIRS)

SHELLSCRIPTS = notion-lock

TARGETS =

######################################

include $(TOPDIR)/build/rules.mk

######################################

_install:
	$(INSTALLDIR) $(DESTDIR)$(SHAREDIR)
	for i in $(SHELLSCRIPTS); do \
		$(INSTALL) -m $(BIN_MODE) $$i $(DESTDIR)$(SHAREDIR); \
	done

%: %.in
	sed 's#@SHAREDIR@#$(SHAREDIR)#g' $< | \
	sed 's#@VARDIR@#$(VARDIR)#g' > $@
	chmod $(BIN_MODE) $@
