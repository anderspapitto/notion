#include <string.h>
#include <ctype.h>
#include <dlfcn.h>
#include <unistd.h>

#include <libtu/rb.h>
#include <libextl/readconfig.h>

#include "common.h"
#include "modules.h"
#include "../version.h"

typedef void *dlhandle;

static Rb_node modules = NULL;

static dlhandle get_handle(const char *name) {
  int found = 0;
  Rb_node nd;

  nd = rb_find_key_n(modules, name, &found);
  if (found) return nd->v.val;
  return NULL;
}

static const char *get_name(dlhandle handle) {
  Rb_node nd;
  rb_traverse(nd, modules) {
    if (nd->v.val == handle) return (const char *)(nd->k.key);
  }
  return NULL;
}

static Rb_node add_module(char *name, dlhandle handle) {
  return rb_insert(modules, name, handle);
}

static void *get_module_symbol(dlhandle handle, const char *modulename,
                               const char *name) {
  char *p;
  void *ret;
  p = scat(modulename, name);
  if (p == NULL) return NULL;
  ret = dlsym(handle, p);
  free(p);
  return ret;
}

static void (*get_module_fptr(dlhandle handle, const char *modulename, const char *name))(void **) {
  /* This is not valid ISO C. However, it is 'tried and tested'. The
   * workaround originally recommended[1] is not alias-safe[2]. The approach
   * we chose, while not valid ISO C, *is* valid under POSIX 2008[3]. Newer
   * versions of GCC should not warn about it anymore[4].
   *
   * [1]
   *http://pubs.opengroup.org/onlinepubs/009695399/functions/dlsym.html#tag_03_112_06)
   * [2] http://gcc.gnu.org/bugzilla/show_bug.cgi?id=45289#c1
   * [3]
   *http://pubs.opengroup.org/onlinepubs/9699919799/functions/V2_chap02.html#tag_15_12_03
   * [4] http://gcc.gnu.org/bugzilla/show_bug.cgi?id=45289#c10
   */
  return (void (*)(void **))get_module_symbol(handle, modulename, name);
}

static char *get_version(dlhandle handle, const char *modulename) {
  return (char *)get_module_symbol(handle, modulename, "_ion_api_version");
}

static bool check_has_version(dlhandle handle, const char *modulename) {
  return get_version(handle, modulename) != NULL;
}

static bool check_version(dlhandle handle, const char *modulename) {
  char *versionstr = get_version(handle, modulename);
  if (versionstr == NULL) return FALSE;
  return (strcmp(versionstr, NOTION_API_VERSION) == 0);
}

static bool call_init(dlhandle handle, const char *modulename) {
  bool (*initfn)(void);

  initfn = (bool (*)())get_module_fptr(handle, modulename, "_init");

  if (initfn == NULL) return TRUE;

  return initfn();
}

bool ioncore_init_module_support() {
  modules = make_rb();
  return (modules != NULL);
}

static int try_load(const char *file, void *UNUSED(param)) {
  dlhandle handle = NULL;
  const char *slash, *dot;
  char *name;
  Rb_node mod;

  if (access(file, F_OK) != 0) return EXTL_TRYCONFIG_NOTFOUND;

  slash = strrchr(file, '/');
  dot = strrchr(file, '.');

  if (slash == NULL)
    slash = file;
  else
    slash++;

  if (dot <= slash) {
    warn("Invalid module name.");
    goto err1;
  }

  name = ALLOC_N(char, dot - slash + 1);
  if (name == NULL) goto err1;

  strncpy(name, slash, dot - slash);
  name[dot - slash] = '\0';

  if (get_handle(name)) {
    warn_obj(file, "The module is already loaded.");
    goto err2;
  }

  handle = dlopen(file, RTLD_NOW | RTLD_GLOBAL);

  if (handle == NULL) {
    warn_obj(file, "%s", dlerror());
    goto err2;
  }

  if (get_name(handle)) return EXTL_TRYCONFIG_OK;

  if (!check_has_version(handle, name)) {
    warn_obj(file, "Module version information for %s not found. "
                   "Refusing to use.",
             name);
    goto err3;
  }

  if (!check_version(handle, name)) {
    warn_obj(file, "Module version mismatch: expected '%s', found '%s'."
                   " Refusing to use.",
             NOTION_API_VERSION, get_version(handle, name));
    goto err3;
  }

  mod = add_module(name, handle);

  if (mod == NULL) goto err3;

  if (!call_init(handle, name)) {
    warn_obj(file, "Unable to initialise module %s.", name);
    rb_delete_node(mod);
    goto err3;
  }

  return EXTL_TRYCONFIG_OK;

err3:
  dlclose(handle);
err2:
  free(name);
err1:
  return EXTL_TRYCONFIG_LOAD_FAILED;
}

static bool do_load_module(const char *modname) {
  int retval;
  const char *extension = "so";

  retval = extl_try_config(modname, NULL, (ExtlTryConfigFn *)try_load, NULL, extension, NULL);

  if (retval == EXTL_TRYCONFIG_NOTFOUND) {
    warn("Unable to find '%s.%s' on search path.", modname, extension);
  }

  return (retval == EXTL_TRYCONFIG_OK);
}

EXTL_EXPORT
bool ioncore_load_module(const char *modname) {
  if (modname == NULL) {
    warn("No module to load given.");
    return FALSE;
  }
  return do_load_module(modname);
}
