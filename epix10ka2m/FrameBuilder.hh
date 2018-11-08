#ifndef Pds_Epix10ka2m_FrameBuilder_hh
#define Pds_Epix10ka2m_FrameBuilder_hh

#include "pdsdata/xtc/XtcIterator.hh"
#include "pds/config/EpixConfigType.hh"
#include "pds/config/EpixDataType.hh"
#include "ndarray/ndarray.h"

namespace Pds {
  namespace Epix10ka2m {
    class FrameBuilder : public Pds::XtcIterator {
    public:
      FrameBuilder(const Xtc* in, 
                   Xtc*       out, 
                   const Epix10kaQuadConfigType&);
      FrameBuilder(const Xtc* in, 
                   Xtc*       out, 
                   const Epix10ka2MConfigType&);
    public:
      int process(Xtc*);
    public:
      unsigned payloadSize() const { return _alloc; }
    private:
      Epix10kaDataArray*        _payload;
      ndarray<const uint16_t,3> _array;
      ndarray<const uint16_t,3> _calib;
      ndarray<const uint32_t,3> _env;
      //      ndarray<const uint16_t,2> _temp;
      unsigned                  _alloc;
      Xtc*                      _next;  // additional payload
    };
  };
};

#endif
