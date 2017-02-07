PREFIX=/usr/local/

BINDIR=$(PREFIX)/bin
ETCDIR=$(PREFIX)/etc
MANDIR=$(PREFIX)/man
DOCDIR=$(PREFIX)/doc
LIBDIR=$(PREFIX)/lib
INCDIR=$(PREFIX)/include

CC=gcc

C89_SOURCE=-ansi

# Same as '-Wall -pedantic' without '-Wunused' as callbacks often
# have unused variables.
WARN=	-W -Wimplicit -Wreturn-type -Wswitch -Wcomment \
	-Wtrigraphs -Wformat -Wchar-subscripts \
	-Wparentheses -pedantic -Wuninitialized

CFLAGS= -Os $(WARN) $(DEFINES) $(INCLUDES) $(EXTRA_INCLUDES)
LDFLAGS= $(LIBS) $(EXTRA_LIBS)

AR=ar
ARFLAGS=cr
RANLIB=ranlib

# Should work almost everywhere
INSTALL=sh $(TOPDIR)/install-sh -c
INSTALLDIR=mkdir -p

BIN_MODE=755
DATA_MODE=664

STRIP=strip
