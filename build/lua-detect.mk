LUA_VERSION=

LUA_LIBS     := $(or $(shell pkg-config --libs lua$(LUA_VERSION)),   $(error "pkg-config couldn't find linker flags for lua$(LUA_VERSION)!"))
LUA_INCLUDES := $(shell pkg-config --cflags lua$(LUA_VERSION))
LUA          := $(or $(shell which lua$(LUA_VERSION)),               $(error No lua$(LUA_VERSION) interpreter found!))
LUAC         := $(or $(shell which luac$(LUA_VERSION)),              $(error No lua$(LUA_VERSION) compiler found!))
