#ifndef Pds_Vimba_FrameBuffer_hh
#define Pds_Vimba_FrameBuffer_hh

#include "vimba/include/VimbaC.h"

namespace Pds {
  namespace Vimba {
    class Camera;
    class Server;
    class FrameBuffer {
      public:
        FrameBuffer(unsigned nbuffers, Camera* cam, Server* srv);
        virtual ~FrameBuffer();

        void configure();
        void unconfigure();

        bool enable();
        bool disable();

        static VmbUint32_t sizeAs16Bit(VmbFrame_t* frame);
        static bool copyAs16Bit(VmbFrame_t* frame, void* buffer);
        static void VMB_CALL frameCallBack(const VmbHandle_t hcam, VmbFrame_t* ptr);
      protected:
        static bool copyAs16BitVmb(VmbFrame_t* frame, void* buffer);
        static bool copyAs16BitStd(VmbFrame_t* frame, void* buffer);
      private:
        unsigned    _nbuffers;
        Camera*     _cam;
        Server*     _srv;
        VmbFrame_t* _frames;
        char*       _buffer;
        VmbInt64_t  _buffer_sz;
    };
  }
}

#endif
