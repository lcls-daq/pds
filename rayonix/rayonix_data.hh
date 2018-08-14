// $Id$
// Author: Chris Ford <caf@slac.stanford.edu>

#ifndef __RAYONIX_DATA_HH
#define __RAYONIX_DATA_HH

#include "rayonix_common.hh"

#define UDP_RCVBUF_SIZE     (64*1024*1024)

namespace Pds
{
  class rayonix_data;

  namespace Rayonix_MX170HS {
    enum { depth_bits = 16 };
    enum { depth_bytes = 2 };
    enum { n_pixels = 14745600 };
    enum { n_pixels_fast = 3840 };
    enum { n_pixels_slow = 3840 };
    enum { n_sensors = 4 };
    enum { n_sensors_fast = 2 };
    enum { n_sensors_slow = 2 };
    enum { offset = 0 };
    enum { min_binning_f = 2, max_binning_f = 10 };
    enum { min_binning_s = 2, max_binning_s = 10 };
  }

  namespace Rayonix_MX340HS {
    enum { depth_bits = 16 };
    enum { depth_bytes = 2 };
    enum { n_pixels = 58982400 };
    enum { n_pixels_fast = 7680 };
    enum { n_pixels_slow = 7680 };
    enum { n_sensors = 16 };
    enum { n_sensors_fast = 2 };
    enum { n_sensors_slow = 2 };
    enum { offset = 0 };
    enum { min_binning_f = 2, max_binning_f = 10 };
    enum { min_binning_s = 2, max_binning_s = 10 };
  }

  namespace Rayonix_Info {
    enum Model { MX170HS=0, MX340HS=1 };
  }
}

class Pds::rayonix_data {

public:
  rayonix_data(bool verbose);
  ~rayonix_data();

  int fd() const                  { return (_notifyFd); }
  int drainFd(int fd) const;
  int reset(bool verbose) const;
  int readFrame(uint16_t& frameNumber, char *payload, int payloadMax, int &binning_f, int &binning_s, int &scaling, bool verbose) const;
  Rayonix_Info::Model getDetectorModel(const char* device_id) const;

  enum { DiscardBufSize = 10000 };

private:
  //
  // private variables
  //
  int         _notifyFd;
  int         _dataFdEven;
  int         _dataFdOdd;
  char *      _discard;
  unsigned    _bufsize;
};
  
#endif
