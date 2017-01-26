include build/system-inc.mk
include modulelist.mk

INSTALL_SUBDIRS= $(MODULE_LIST) ioncore notion
SUBDIRS = $(LIBS_SUBDIRS) $(INSTALL_SUBDIRS)
DOCS = README.md LICENSE 
TO_REALCLEAN = build/ac/system-ac.mk

include build/rules.mk

_install:
	$(INSTALLDIR) $(DESTDIR)$(DOCDIR)
	for i in $(DOCS); do \
		$(INSTALL) -m $(DATA_MODE) $$i $(DESTDIR)$(DOCDIR); \
	done

relocatable_build:
	$(MAKE) RELOCATABLE=1 PREFIX=

.PHONY: test

test:
	$(MAKE) -C mod_xrandr test
	$(MAKE) -C test
