PREFIX=/usr/local

# Main binaries
BINDIR=$(PREFIX)/bin
# Configuration .lua files
ETCDIR=$(PREFIX)/etc/libextl
# Some .lua files and ion-* shell scripts
SHAREDIR=$(PREFIX)/share/libextl
# Manual pages
MANDIR=$(PREFIX)/share/man
# Some documents
DOCDIR=$(PREFIX)/share/doc/libextl
# Nothing at the moment
INCDIR=$(PREFIX)/include/libextl
# Nothing at the moment
LIBDIR=$(PREFIX)/lib
# Modules
MODULEDIR=$(LIBDIR)/libextl/mod
# Compiled Lua source code
LCDIR=$(LIBDIR)/libextl/lc
# ion-completefile (does not belong in SHAREDIR being a binary file)
EXTRABINDIR=$(LIBDIR)/libextl/bin
# For ion-completeman system-wide cache
VARDIR=/var/cache/libextl
# Message catalogs
LOCALEDIR=$(PREFIX)/share/locale

LIBTU_LIBS = -ltu
LIBTU_INCLUDES =

# To skip auto-detection of lua uncomment this and edit the variables below to
# suit your installation of lua.
#LUA_MANUAL=1

# Default to paths and names that should work for a build installed from the
# official Lua 5.1 source tarball.
LUA_DIR=$(shell dirname `which lua` | xargs dirname)
LUA_LIBS=-L$(LUA_DIR)/lib -llua
LUA_INCLUDES = -I$(LUA_DIR)/include
LUA=$(LUA_DIR)/bin/lua
LUAC=$(LUA_DIR)/bin/luac

# Attempt to autodect lua using pkg-config.

ifndef LUA_MANUAL

ifeq (5.2,$(findstring 5.2,$(shell pkg-config --exists lua5.2 && pkg-config --modversion lua5.2)))

LUA_LIBS=`pkg-config --libs lua5.2`
LUA_INCLUDES=`pkg-config --cflags lua5.2`
LUA=`which lua5.2`
LUAC=`which luac5.2`

else ifeq (5.1,$(findstring 5.1,$(shell pkg-config --exists lua5.1 && pkg-config --modversion lua5.1)))

LUA_LIBS=`pkg-config --libs lua5.1`
LUA_INCLUDES=`pkg-config --cflags lua5.1`
LUA=`which lua5.1`
LUAC=`which luac5.1`

else ifeq (5.1,$(findstring 5.1,$(shell pkg-config --exists lua && pkg-config --modversion lua)))

LUA_LIBS=`pkg-config --libs lua`
LUA_INCLUDES=`pkg-config --cflags lua`
LUA=`which lua`
LUAC=`which luac`

endif # lua

endif # LUA_MANUAL

CC=gcc

# Same as '-Wall -pedantic' without '-Wunused' as callbacks often
# have unused variables.
WARN=-W -Wimplicit -Wreturn-type -Wswitch -Wcomment \
	-Wtrigraphs -Wformat -Wchar-subscripts \
	-Wparentheses -pedantic -Wuninitialized

CFLAGS=-Os $(WARN) $(DEFINES) $(EXTRA_INCLUDES) $(INCLUDES)
LDFLAGS=-Os $(EXTRA_LIBS) $(LIBS)

C99_SOURCE=-std=gnu11

AR=ar
ARFLAGS=cr
RANLIB=ranlib

INSTALL=sh $(TOPDIR)/install-sh -c
INSTALLDIR=mkdir -p

BIN_MODE=755
DATA_MODE=644

STRIP=strip

RM=rm
