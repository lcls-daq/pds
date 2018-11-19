#ifndef Pds_Epix10ka2m_FrameBuilder_hh
#define Pds_Epix10ka2m_FrameBuilder_hh

#include "pdsdata/xtc/XtcIterator.hh"
#include "pds/config/EpixConfigType.hh"
#include "pds/config/EpixDataType.hh"
#include "ndarray/ndarray.h"

namespace Pds {
  class Datagram;
  namespace Epix10ka2m {
    class FrameBuilder : public Pds::XtcIterator {
    public:
      FrameBuilder(const Datagram& in, 
                   Datagram&       out, 
                   const Epix10kaQuadConfigType&);
      FrameBuilder(const Datagram& in, 
                   Datagram&       out, 
                   const Epix10ka2MConfigType&);
    public:
      int process(Xtc*);
    private:
      void _add_quad(unsigned quad);
    private:
      unsigned                  _qmask;
      Xtc*                      _qxtc[4];
      Epix10kaDataArray*        _payload;
      ndarray<const uint16_t,3> _array;
      ndarray<const uint16_t,3> _calib;
      ndarray<const uint32_t,3> _env;
      //      ndarray<const uint16_t,2> _temp;
      Xtc*                      _next;  // additional payload
    };
  };
};

#endif
