##
## Notion manual page Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/build/system-inc.mk

######################################

TRANSLATIONS=fi cs

WELCOME_TARGETS=\
	welcome.txt \
	$(foreach tr, $(TRANSLATIONS), welcome.$(tr).txt)

TARGETS=notion.1 notionflux.1 $(foreach tr, $(TRANSLATIONS), notion.$(tr).1) \
	$(WELCOME_TARGETS)

MKMAN=$(LUA) ../build/mkman.lua $(MKMAN_DEFS)
MKMAN_DEFS=-D DOCDIR $(REL)$(DOCDIR)

ifeq ($(RELOCATABLE),1)
REL="/..."
endif

NROFF=nroff -man -Tlatin1
#FILTERCRAP=perl -p -i -e 's/.\10//g'
FILTERCRAP=$(LUA) -e 'io.write((string.gsub(io.read("*a"), ".\8", "")))'
######################################

include $(TOPDIR)/build/rules.mk

######################################

notion.1: notion.in $(CONFIGS)
	$(MKMAN) -i $< -o $@ $(CONFIGS)

notion.%.1: notion.%.in $(CONFIGS) ../po/%.po
	$(MKMAN) -po ../po/$*.po -i $< -o $@ $(CONFIGS)

notionflux.1: notionflux.in $(CONFIGS)
	$(MKMAN) -i $< -o $@ $(CONFIGS)

welcome%txt: welcome%head notion%1
	(cat welcome$*head; \
	$(NROFF) notion$*1 | $(FILTERCRAP)) > $@

_install:
	$(INSTALLDIR) $(DESTDIR)$(MANDIR)/man1
	$(INSTALL) -m $(DATA_MODE) notion.1 $(DESTDIR)$(MANDIR)/man1
	$(INSTALL) -m $(DATA_MODE) notionflux.1 $(DESTDIR)$(MANDIR)/man1
	for tr in $(TRANSLATIONS); do \
		$(INSTALLDIR) $(DESTDIR)$(MANDIR)/$$tr/man1 ; \
		$(INSTALL) -m $(DATA_MODE) notion.$$tr.1 $(DESTDIR)$(MANDIR)/$$tr/man1/notion.1 ; \
	done
	$(INSTALLDIR) $(DESTDIR)$(SHAREDIR)
	for i in $(WELCOME_TARGETS); do \
		$(INSTALL) -m $(DATA_MODE) $$i $(DESTDIR)$(SHAREDIR) ; \
	done
