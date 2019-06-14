#include "pds/service/PathTools.hh"

#include <unistd.h>
#include <limits.h>
#include <libgen.h>
#include <stdio.h>
#include <string.h>

using namespace Pds;

static bool getRelativePath(unsigned depth, const char* relative, char* buf, size_t bufsz)
{
  char modpath[PATH_MAX]; // path to be modified by dirname/basename
  char dirpath[PATH_MAX]; // assemble the directory path here

  ssize_t ret = ::readlink("/proc/self/exe", modpath, PATH_MAX - 1);
  if (ret != -1) {
    modpath[ret] = 0;
    char* dir = dirname(modpath);
    if (strlen(dir) < PATH_MAX) {
      ::strcpy(dirpath, dir);
      ::strcpy(modpath, dir);
      char* arch = basename(modpath);
      for (unsigned i=0; i<depth; i++) {
        ::strcpy(modpath, dirpath);
        dir = dirname(modpath);
        ::strcpy(dirpath, dir);
      }
      if (relative) {
        size_t len = strlen(dirpath);
        if ((strlen(relative) + strlen(arch)) < (PATH_MAX - len)) {
          ::strcpy(dirpath + len, relative);
          ::strcpy(dirpath + strlen(dirpath), arch);
        } else {
          return false;
        }
      }

      if (strlen(dirpath) < bufsz) {
        ::strcpy(buf, dirpath);
        return true;
      }
    }
  }

  return false;
}

bool PathTools::getBuildPath(char* buf, size_t bufsz)
{
  return getRelativePath(3, NULL, buf, bufsz);
}

bool PathTools::getReleasePath(char* buf, size_t bufsz)
{
  return getRelativePath(4, NULL, buf, bufsz);
}

bool PathTools::getLibPath(const char* package, char* buf, size_t bufsz)
{
  char relpath[strlen(package) + 7];
  sprintf(relpath, "/%s/lib/", package);
  return getRelativePath(3, relpath, buf, bufsz);
}

bool PathTools::getBinPath(const char* package, char* buf, size_t bufsz)
{
  char relpath[strlen(package) + 7];
  sprintf(relpath, "/%s/bin/", package);
  return getRelativePath(3, relpath, buf, bufsz);
}

std::string PathTools::getBuildPathStr()
{
  char dirpath[PATH_MAX];
  if (getBuildPath(dirpath, PATH_MAX))
    return std::string(dirpath);
  else
    return std::string();
}

std::string PathTools::getReleasePathStr()
{
  char dirpath[PATH_MAX];
  if (getReleasePath(dirpath, PATH_MAX))
    return std::string(dirpath);
  else
    return std::string();
}

std::string PathTools::getLibPathStr(const char* package)
{
  char dirpath[PATH_MAX];
  if (getLibPath(package, dirpath, PATH_MAX))
    return std::string(dirpath);
  else
    return std::string();
}

std::string PathTools::getBinPathStr(const char* package)
{
  char dirpath[PATH_MAX];
  if (getBinPath(package, dirpath, PATH_MAX))
    return std::string(dirpath);
  else
    return std::string();
}
