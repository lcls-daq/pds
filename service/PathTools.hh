#ifndef Pds_PathTools_hh
#define Pds_PathTools_hh

#include <string>

namespace Pds {
  class PathTools {
  public:
    static bool getBuildPath(char* buf, size_t bufsz);
    static bool getReleasePath(char* buf, size_t bufsz);
    static bool getLibPath(const char* package, char* buf, size_t bufsz);
    static bool getBinPath(const char* package, char* buf, size_t bufsz);
    static std::string getBuildPathStr();
    static std::string getReleasePathStr();
    static std::string getLibPathStr(const char* package);
    static std::string getBinPathStr(const char* package);
  };
};

#endif
