#include "pds/service/PathTools.hh"
#include "pds/service/Semaphore.hh"

#include <unistd.h>
#include <limits.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sstream>

using namespace Pds;

static const unsigned BUILD_DEPTH = 3;
static const unsigned RELEASE_DEPTH = 4;
static bool py_init_needed = true;
static Semaphore py_init_sem(Semaphore::FULL);

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
  return getRelativePath(BUILD_DEPTH, NULL, buf, bufsz);
}

bool PathTools::getReleasePath(char* buf, size_t bufsz)
{
  return getRelativePath(RELEASE_DEPTH, NULL, buf, bufsz);
}

bool PathTools::getLibPath(const char* package, char* buf, size_t bufsz)
{
  char relpath[strlen(package) + 7];
  sprintf(relpath, "/%s/lib/", package);
  return getRelativePath(BUILD_DEPTH, relpath, buf, bufsz);
}

bool PathTools::getBinPath(const char* package, char* buf, size_t bufsz)
{
  char relpath[strlen(package) + 7];
  sprintf(relpath, "/%s/bin/", package);
  return getRelativePath(BUILD_DEPTH, relpath, buf, bufsz);
}

bool PathTools::getPythonPath(char* buf, size_t bufsz)
{
  return getRelativePath(BUILD_DEPTH, "/pyenv/", buf, bufsz);
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

std::string PathTools::getPythonPathStr()
{
  char dirpath[PATH_MAX];
  if (getPythonPath(dirpath, PATH_MAX))
    return std::string(dirpath);
  else
    return std::string();
}

void PathTools::initPythonPath()
{
  py_init_sem.take();
  if (py_init_needed) {
    std::string python_path = getPythonPathStr();
    char* old_python_path = getenv("PYTHONPATH");
    if (old_python_path) {
      std::stringstream ss;
      ss << python_path << ":" << old_python_path;
      python_path = ss.str();
    }
    setenv("PYTHONPATH", python_path.c_str(), true);
    py_init_needed = false;
  }
  py_init_sem.give();
}
