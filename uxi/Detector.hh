#ifndef Pds_Uxi_Detector_hh
#define Pds_Uxi_Detector_hh

#include <unistd.h>
#include <stdint.h>

namespace Pds {
  namespace Uxi {
    class Detector {
    public:
      Detector(const char* host, unsigned comm_port, unsigned data_port);
      ~Detector();
      void connect();
      void disconnect();
      bool connected() const;
      bool acquire(unsigned nframes=0);
      bool start();
      bool stop();
      bool commit();
      bool temperature(double* temp);
      bool width(uint32_t* width);
      bool height(uint32_t* height);
      bool nframes(uint32_t* nframes);
      bool nbytes(uint32_t* nbytes);
      bool type(uint32_t* type);
      bool acq_count(uint32_t* acq_count);
      bool num_pixels(uint32_t* num_pixels);
      bool frame_size(uint32_t* frame_size);
      bool num_pots(uint32_t* num_pots);
      bool get_pot(unsigned pot, double* value);
      bool set_pot(unsigned pot, double value);
      bool set_timing_all(unsigned ton, unsigned toff, unsigned delay=0);
      bool set_timing(char side, unsigned ton, unsigned toff, unsigned delay=0);
      bool get_timing(char side, unsigned* ton, unsigned* toff, unsigned* delay=NULL);
      bool get_frames(uint32_t& acq_num, uint16_t* data, double* temp=NULL, uint32_t* timestamp=NULL);

      static const int BasePort = 14100;
    private:
      int get_frame_part(uint16_t* data, size_t len);
      bool connect_socket(void* sock, unsigned port);
      bool get_uint32(const char* cmd, uint32_t* value);
      bool get_double(const char* cmd, double* value);
      bool put_command(const char* cmd, void* payload=NULL, size_t size=0);
      bool get_command(const char* cmd, void* payload, size_t size);
    private:
      const char* _host;
      unsigned    _data_port;
      unsigned    _comm_port;
      bool        _data_up;
      bool        _comm_up;
      char*       _header_buf;
      void*       _context;
      void*       _data;
      void*       _comm;
    };
  }
}

#endif
