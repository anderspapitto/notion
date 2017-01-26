PKG_CONFIG ?= pkg-config
PREFIX ?= /usr
PKG_CONFIG ?= pkg-config
BINDIR=$(PREFIX)/bin
SHAREDIR=$(PREFIX)/share/notion
MANDIR=$(PREFIX)/share/man
DOCDIR=$(PREFIX)/share/doc/notion
LIBDIR=$(PREFIX)/lib
MODULEDIR=$(LIBDIR)/notion/mod
LCDIR=$(LIBDIR)/notion/lc
EXTRABINDIR=$(LIBDIR)/notion/bin
VARDIR=/var/cache/notion
LOCALEDIR=$(PREFIX)/share/locale
ETCDIR ?= $(PREFIX)/etc/notion
ifeq ($(PREFIX),)
    INCDIR = $(PREFIX)/include/notion
else
    INCDIR = /usr/include/notion
endif
DL_LIBS=-ldl
include $(TOPDIR)/build/lua-detect.mk
X11_PREFIX ?= /usr
X11_LIBS=-L$(X11_PREFIX)/lib -lX11 -lXext
X11_INCLUDES=-I$(X11_PREFIX)/include
DEFINES += -DCF_XFREE86_TEXTPROP_BUG_WORKAROUND

# You may uncomment this if you know that your system C libary provides
# asprintf and  vasprintf. (GNU libc does.) If HAS_SYSTEM_ASPRINTF is not
# defined, an implementation provided in libtu/sprintf_2.2/ is used. 
HAS_SYSTEM_ASPRINTF ?= 1

# The following setting is needed with GNU libc for clock_gettime and the
# monotonic clock. Other systems may not need it, or may not provide a
# monotonic clock at all (which Ion can live with, and usually detect).
EXTRA_LIBS += -lrt

#CC ?= gcc
CC ?= clang

WARN=-W -Wall -pedantic 

CFLAGS += -Os $(WARN) $(DEFINES) $(INCLUDES) $(EXTRA_INCLUDES) \
          -DHAS_SYSTEM_ASPRINTF=$(HAS_SYSTEM_ASPRINTF)

LDFLAGS += -Wl,--as-needed $(LIBS) $(EXTRA_LIBS)
EXPORT_DYNAMIC=-Xlinker --export-dynamic

POSIX_SOURCE?=-D_POSIX_C_SOURCE=200112L
BSD_SOURCE?=-D_BSD_SOURCE

XOPEN_SOURCE=-D_XOPEN_SOURCE -D_XOPEN_SOURCE_EXTENDED
C99_SOURCE?=-std=gnu11 -DCF_HAS_VA_COPY

AR ?= ar
ARFLAGS ?= cr
RANLIB ?= ranlib

INSTALL ?= sh $(TOPDIR)/install-sh -c
INSTALL_STRIP = -s
INSTALLDIR ?= mkdir -p

BIN_MODE ?= 755
DATA_MODE ?= 644

RM ?= rm

## Debugging
INSTALL_STRIP =
CFLAGS += -g

ifeq ($(PRELOAD_MODULES),1)
X11_LIBS += -lXinerama -lXrandr
endif

USE_XFT=1

ifeq ($(USE_XFT),1)
    X11_INCLUDES += `pkg-config xft --cflags`
    X11_LIBS += `pkg-config xft --libs`
    DEFINES += -DHAVE_X11_XFT
endif
