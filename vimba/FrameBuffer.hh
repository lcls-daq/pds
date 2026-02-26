#ifndef Pds_Vimba_FrameBuffer_hh
#define Pds_Vimba_FrameBuffer_hh

#include "VmbC/VmbC.h"

namespace Pds {
  namespace Vimba {
    class Camera;
    class Server;
    class FrameBuffer {
      public:
        FrameBuffer(size_t nbuffers, Camera* cam, Server* srv);
        virtual ~FrameBuffer();

        void configure();
        void unconfigure();

        bool enable();
        bool disable();

        static VmbUint32_t sizeAs16Bit(VmbFrame_t* frame);
        static bool copyAs16Bit(VmbFrame_t* frame, void* buffer);
        static void VMB_CALL frameCallBack(const VmbHandle_t hcam, const VmbHandle_t hstream, VmbFrame_t* ptr);
      protected:
        static bool copyAs16BitVmb(VmbFrame_t* frame, void* buffer);
        static bool copyAs16BitStd(VmbFrame_t* frame, void* buffer);
      private:
        size_t       _nbuffers;
        Camera*      _cam;
        Server*      _srv;
        VmbFrame_t*  _frames;
        char*        _buffer;
        size_t       _buffer_sz;
    };
  }
}

#endif
