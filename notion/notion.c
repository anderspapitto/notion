#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <libtu/util.h>
#include <libtu/errorlog.h>
#include <libtu/prefix.h>
#include <libextl/readconfig.h>
#include <libmainloop/exec.h>

#include <ioncore/common.h>
#include <ioncore/global.h>
#include <ioncore/ioncore.h>
#include <ioncore/exec.h>
#include <ioncore/event.h>
#include "../version.h"

#ifndef P_tmpdir
#define P_tmpdir "/tmp"
#endif

void check_new_user_help() {
  const char *userdir = extl_userdir();
  char *oldbeard = NULL;
  char *tmp = NULL, *cmd = NULL;
  bool ret;

  if (userdir == NULL) {
    warn("Could not get user configuration file directory.");
    return;
  }

  libtu_asprintf(&oldbeard, "%s/.welcome_msg_displayed", userdir);

  if (oldbeard == NULL) return;

  if (access(oldbeard, F_OK) == 0) {
    free(oldbeard);
    return;
  }

  if (tmp != NULL) {
    if (access(tmp, F_OK) == 0)
      libtu_asprintf(&cmd, "%s %s", CF_XMESSAGE, tmp);
    else
      libtu_asprintf(&cmd, "%s %s/welcome.txt", CF_XMESSAGE, SHAREDIR);

    free(tmp);

    if (cmd != NULL) {
      ret = ioncore_exec(cmd);

      free(cmd);

      if (ret) {
        /* This should actually be done when less or xmessage returns,
         * but that would mean yet another script...
         */
        mkdir(userdir, 0700);
        if (open(oldbeard, O_CREAT | O_RDWR, 0600) < 0) warn_err_obj(oldbeard);
      }
    }
  }

  free(oldbeard);
}
int main(int argc, char *argv[]) {
  const char *cfgfile = "cfg_notion";
  const char *display = NULL;
  char *cmd = NULL;
  int stflags = 0;
  ErrorLog el;
  FILE *ef = NULL;
  char *efnam = NULL;
  bool may_continue = FALSE;
  bool noerrorlog = FALSE;
  char *localedir;

  libtu_init(argv[0]);

#ifdef CF_RELOCATABLE_BIN_LOCATION
  prefix_set(argv[0], CF_RELOCATABLE_BIN_LOCATION);
#endif

  localedir = prefix_add(LOCALEDIR);

  if (!ioncore_init(CF_EXECUTABLE, argc, argv)){
      return EXIT_FAILURE;
  }

  if (localedir != NULL) free(localedir);

  prefix_wrap_simple(extl_add_searchdir, EXTRABINDIR); /* ion-completefile */
  prefix_wrap_simple(extl_add_searchdir, MODULEDIR);
  prefix_wrap_simple(extl_add_searchdir, ETCDIR);
  prefix_wrap_simple(extl_add_searchdir, SHAREDIR);
  prefix_wrap_simple(extl_add_searchdir, LCDIR);
  extl_set_userdirs(CF_EXECUTABLE);

  if (!noerrorlog) {
    /* We may have to pass the file to xmessage so just using tmpfile()
     * isn't sufficient.
     */
    libtu_asprintf(&efnam, "%s/ion-%d-startup-errorlog", P_tmpdir, getpid());
    if (efnam == NULL) {
      warn_err();
    } else {
      ef = fopen(efnam, "wt");
      if (ef == NULL) {
        warn_err_obj(efnam);
        free(efnam);
        efnam = NULL;
      } else {
        cloexec_braindamage_fix(fileno(ef));
        fprintf(ef, "Notion startup error log:\n");
        errorlog_begin_file(&el, ef);
      }
    }
  }

  if (ioncore_startup(display, cfgfile, stflags)) may_continue = TRUE;

  if (!may_continue)
    warn("Refusing to start due to encountered errors.");
  else
    check_new_user_help();

  if (ef != NULL) {
    pid_t pid = -1;
    if (errorlog_end(&el) && ioncore_g.dpy != NULL) {
      fclose(ef);
      pid = fork();
      if (pid == 0) {
        ioncore_setup_display(DefaultScreen(ioncore_g.dpy));
        if (!may_continue)
          XCloseDisplay(ioncore_g.dpy);
        else
          close(ioncore_g.conn);
        libtu_asprintf(&cmd, CF_XMESSAGE " %s", efnam);
        if (cmd == NULL) {
          warn_err();
        } else if (system(cmd) == -1) {
          warn_err_obj(cmd);
        }
        unlink(efnam);
        exit(EXIT_SUCCESS);
      }
      if (!may_continue && pid > 0) waitpid(pid, NULL, 0);
    } else {
      fclose(ef);
    }
    if (pid < 0) unlink(efnam);
    free(efnam);
  }

  if (!may_continue) return EXIT_FAILURE;

  ioncore_mainloop();

  /* The code should never return here */
  return EXIT_SUCCESS;
}
