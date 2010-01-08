/* Copyright (C) 2009 - Virtualsquare Team
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#include <vde3.h>

#include <vde3/module.h>

#include <dlfcn.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <fts.h>

// defined here, including contex.h would required module.h in turn
extern int vde_context_register_module(vde_context *ctx, vde_module *module);

vde_component_kind vde_module_get_kind(vde_module *module)
{
  return module->kind;
}

const char *vde_module_get_family(vde_module *module)
{
  return module->family;
}

component_ops *vde_module_get_component_ops(vde_module *module)
{
  return module->cops;
}

/**
 * @brief Try loading a vde_module from path and register to context
 *
 * @param ctx The context to register the module in
 * @param path The path to a file to load the module from
 *
 * @return 0 on success, -1 on error (and errno set appropriately)
 */
static int module_try_load(vde_context *ctx, char *path)
{
  void *handle;
  int rv = 0, tmp_errno;
  char *last_dlerror;
  vde_module *mod;

  handle = dlopen(path, RTLD_LAZY | RTLD_GLOBAL);
  if (!handle) {
    vde_warning("%s: dlopen error: %s", __PRETTY_FUNCTION__, dlerror());
    errno = EINVAL;
    return -1;
  }

  dlerror(); // reset error
  mod = (vde_module *)dlsym(handle, VDE_MODULE_START_S);
  last_dlerror = dlerror();
  if (last_dlerror) {
    vde_warning("%s: dlsym error: %s", __PRETTY_FUNCTION__, last_dlerror);
    errno = ENOENT;
    rv = -1;
    goto cleanup;
  }

  if (!mod) {
    vde_warning("%s: symbol %s is NULL", __PRETTY_FUNCTION__,
                VDE_MODULE_START_S);
    errno = EINVAL;
    rv = -1;
    goto cleanup;
  }

  rv = vde_context_register_module(ctx, mod);
  if (rv) {
    tmp_errno = errno;
    vde_error("%s: unable to register module to context", __PRETTY_FUNCTION__);
    errno = tmp_errno;
    rv = -1;
    goto cleanup;
  }

  mod->dlhandle = handle;
  rv = 0;
  goto exit;

cleanup:
  dlclose(handle);
exit:
  return rv;
}

#ifdef VDE_DEFAULT_MODULES_PATH
char *default_modules_path[] = VDE_DEFAULT_MODULES_PATH;
#else
char *default_modules_path[] = {".", NULL};
#endif

// XXX move this elsewhere?
char **vde_modules_default_path() {
  return default_modules_path;
}

int vde_modules_load(vde_context *ctx, char **path)
{
  FTS *ftsp;
  FTSENT *p;

  // symlinks in root path are followed
  // follow symlinks
  // don't chdir
  // don't stat
  int fts_options = FTS_COMFOLLOW | FTS_LOGICAL | FTS_NOCHDIR | FTS_NOSTAT;
  int rv = 0;
  int tmp_errno;

  if (!path) {
    path = vde_modules_default_path();
  }

  // XXX define an array of builtin init functions to call here?
  ftsp = fts_open(path, fts_options, NULL);
  if (!ftsp) {
    tmp_errno = errno;
    vde_error("%s: unable to open modules path", __PRETTY_FUNCTION__);
    errno = tmp_errno;
    return -1;
  }

  while ((p = fts_read(ftsp)) != NULL) {
    switch (p->fts_info) {
    case FTS_F:
      // fts_path has the path prependend
      //vde_debug("examining file %s", p->fts_path);
      if (p->fts_pathlen > 3 &&
          strncmp(p->fts_path + p->fts_pathlen - 3, ".so", 3) == 0 &&
          access(p->fts_path, R_OK) == 0) {
        vde_debug("loading module %s", p->fts_path);

        // XXX define semantics for final rv, see include/vde3/module.h
        module_try_load(ctx, p->fts_path);
      }
      break;
    case FTS_ERR:
      // XXX check error during traversal
      break;
    case FTS_D:
      //vde_debug("examining dir %s", p->fts_path);

      // do not recurse into subdirs
      if (p->fts_level > 0) {
        fts_set(ftsp, p, FTS_SKIP);
      }
      break;
    default:
      break;
    }
  }

  if(fts_close(ftsp)) {
    tmp_errno = errno;
    vde_error("%s: error while closing module path", __PRETTY_FUNCTION__);
    errno = tmp_errno;
    return -1;
  }

  return rv;
}
