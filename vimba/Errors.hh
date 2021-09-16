#ifndef Pds_Vimba_Errors_hh
#define Pds_Vimba_Errors_hh

#include "vimba/include/VimbaC.h"

namespace Pds {
  namespace Vimba {
    class Bool {
    public:
      static const char *name(VmbBool_t value);
      static const char *desc(VmbBool_t value);
    };

    class ErrorCodes {
    public:
      static const char *name(VmbError_t err);
      static const char *desc(VmbError_t err);
    };

    class FrameStatusCodes {
    public:
      static const char *name(VmbFrameStatus_t status);
      static const char *desc(VmbFrameStatus_t status);
    };

    class PixelFormatTypes {
    public:
      static const char *name(VmbPixelFormat_t format);
      static const char *desc(VmbPixelFormat_t format);
    };
  }
}

#endif
