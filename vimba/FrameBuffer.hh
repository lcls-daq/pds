#ifndef Pds_Vimba_FrameBuffer_hh
#define Pds_Vimba_FrameBuffer_hh

#include "pds/service/Semaphore.hh"

#include "VmbC/VmbC.h"

namespace Pds {
  namespace Vimba {
    class Camera;
    class Server;
    class FrameBuffer {
      public:
        FrameBuffer(size_t nbuffers, Camera* cam);
        virtual ~FrameBuffer();

        virtual void configure();
        virtual void unconfigure();

        virtual bool enable();
        virtual bool disable();

        virtual void process(VmbFrame_t* frame) = 0;

        static VmbUint32_t sizeAs16Bit(VmbFrame_t* frame);
        static bool copyAs16Bit(VmbFrame_t* frame, void* buffer);
        static void VMB_CALL frameCallBack(const VmbHandle_t hcam, const VmbHandle_t hstream, VmbFrame_t* ptr);
      protected:
        static bool copyAs16BitVmb(VmbFrame_t* frame, void* buffer);
        static bool copyAs16BitStd(VmbFrame_t* frame, void* buffer);

        Camera*      _cam;
      private:
        size_t       _nbuffers;
        VmbFrame_t*  _frames;
        char*        _buffer;
        size_t       _buffer_sz;
    };

    class SimpleFrameBuffer : public FrameBuffer {
      public:
        SimpleFrameBuffer(size_t nbuffers, size_t nframes, Camera* cam, const char* file_prefix, bool show_stats, bool reformat_pixels);
        virtual ~SimpleFrameBuffer();

        virtual bool enable() override;
        virtual bool disable() override;

        void wait();
        void cancel();

        virtual void process(VmbFrame_t* frame) override;

      private:
        size_t       _nframes;
        size_t       _ncomp;
        const char*  _file_prefix;
        bool         _show_stats;
        bool         _reformat_pixels;
        Semaphore    _sem;
        char*        _fname;
    };

    class ServerFrameBuffer : public FrameBuffer {
      public:
        ServerFrameBuffer(size_t nbuffers, Camera* cam, Server* srv);
        virtual ~ServerFrameBuffer();

        virtual void configure() override;

        virtual void process(VmbFrame_t* frame) override;

      private:
        Server*      _srv;
    };
  }
}

#endif
