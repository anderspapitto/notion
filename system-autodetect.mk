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

EXTRA_LIBS += -lrt

#CC ?= gcc
CC ?= clang

WARN=-W -Wall -pedantic 

CFLAGS += -Os $(WARN) $(INCLUDES) $(EXTRA_INCLUDES)

LDFLAGS += -Wl,--as-needed $(LIBS) $(EXTRA_LIBS)
EXPORT_DYNAMIC=-Xlinker --export-dynamic

POSIX_SOURCE?=-D_POSIX_C_SOURCE=200112L
BSD_SOURCE?=-D_BSD_SOURCE

C99_SOURCE?=-std=gnu11

AR ?= ar
ARFLAGS ?= cr
RANLIB ?= ranlib

INSTALL ?= install
INSTALLDIR ?= mkdir -p

BIN_MODE ?= 755
DATA_MODE ?= 644

RM ?= rm

ifeq ($(PRELOAD_MODULES),1)
	X11_LIBS += -lXinerama -lXrandr
endif

X11_INCLUDES += `pkg-config xft --cflags`
X11_LIBS += `pkg-config xft --libs`
