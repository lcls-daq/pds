#include "pds/epix10ka2m/FrameBuilder.hh"
#include "pds/pgp/DataImportFrame.hh"
#include "pds/pgp/Pgp.hh"
#include "pds/xtc/XtcType.hh"
#include "pdsdata/xtc/Xtc.hh"

using namespace Pds::Epix10ka2m;

FrameBuilder::FrameBuilder(const Xtc*                  in, 
                           Xtc*                        out, 
                           const Epix10ka2MConfigType& cfg) :
  Pds::XtcIterator(const_cast<Xtc*>(in)),
  _payload(new (out->next()) Epix10kaDataArray),
  _array  (_payload->frame            (cfg)),
  _calib  (_payload->calibrationRows  (cfg)),
  _env    (_payload->environmentalRows(cfg))
  //  _temp   (_payload->temperatures     (cfg))
{
  unsigned sz = Epix10kaDataArray::_sizeof(cfg);
  out->alloc(_alloc=sz);
  _next = out->next();

  iterate();
}

FrameBuilder::FrameBuilder(const Xtc*                    in, 
                           Xtc*                          out, 
                           const Epix10kaQuadConfigType& cfg) :
  Pds::XtcIterator(const_cast<Xtc*>(in)),
  _payload(new (out->next()) Epix10kaDataArray),
  _array  (_payload->frame            (cfg)),
  _calib  (_payload->calibrationRows  (cfg)),
  _env    (_payload->environmentalRows(cfg))
  //  _temp   (_payload->temperatures     (cfg))
{
  unsigned sz = Epix10kaDataArray::_sizeof(cfg);
  out->alloc(_alloc=sz);
  _next = out->next();

  iterate();
}

int FrameBuilder::process(Xtc* xtc)
{
  if (xtc->contains.id() == _xtcType.id()) {
    iterate(xtc);
    return 1;
  }

  if (xtc->contains.value() != _epix10kaDataType.value()) {
    memcpy((char*)_next, xtc, xtc->extent);
    _alloc += xtc->extent;
    _next = _next->next();
    return 1;
  }

  //  Set the frame number
  const Pds::Pgp::DataImportFrame* e = reinterpret_cast<Pds::Pgp::DataImportFrame*>(xtc->payload());
  *reinterpret_cast<uint32_t*>(_payload) = e->_frameNumber;

#ifdef DBUG
  printf("%s\n",__PRETTY_FUNCTION__);
  for(unsigned i=0; i<5; i++)
    printf(" %08x",reinterpret_cast<const uint32_t*>(xtc)[i]);
  printf("\n");
  for(unsigned i=0; i<8; i++)
    printf(" %08x",reinterpret_cast<const uint32_t*>(e)[i]);
  printf("\n");
#endif

  //  Each quad arrives on a separate lane
  //  A super row crosses 2 elements; each element contains 2x2 ASICs
  const unsigned lane0        = Pds::Pgp::Pgp::portOffset();
  const unsigned asicRows     = Pds::Epix::Config10ka::_numberOfRowsPerAsic;
  const unsigned elemRowSize  = Pds::Epix::Config10ka::_numberOfAsicsPerRow*Pds::Epix::Config10ka::_numberOfPixelsPerAsicRow;

  // Copy this quadrant into the correct location within cxtc 
  unsigned quad = e->lane()-lane0;

  const uint16_t* u = reinterpret_cast<const uint16_t*>(e+1);
  
#define MMCPY(dst,src,sz) {                                       \
    for(unsigned k=0; k<sz; k++) {                                \
      dst[sz-1-k] = src[k];                                       \
    }                                                             \
    src += sz;                                                    \
  }

#define QCPY(d1,d2,d3,d4,src,sz) {                                \
    for(unsigned k=0; k<sz; k++) {                                \
      d1[sz-1-k] = src[k];                                        \
      d2[sz-1-k] = src[k+sz];                                     \
      d3[sz-1-k] = src[k+sz*2];                                   \
      d4[sz-1-k] = src[k+sz*3];                                   \
    }                                                             \
    src += sz*4;                                                  \
  }

  // Frame data
  for(unsigned i=0; i<asicRows; i++) { // 4 super rows at a time
    unsigned dnRow = asicRows+i;
    unsigned upRow = asicRows-i-1;
    // QCPY(const_cast<uint16_t*>(&_array(4*quad+2,upRow,0)), 
    //      const_cast<uint16_t*>(&_array(4*quad+3,upRow,0)),
    //      const_cast<uint16_t*>(&_array(4*quad+2,dnRow,0)),
    //      const_cast<uint16_t*>(&_array(4*quad+3,dnRow,0)),
    //      u, elemRowSize);
    // QCPY(const_cast<uint16_t*>(&_array(4*quad+0,upRow,0)),
    //      const_cast<uint16_t*>(&_array(4*quad+1,upRow,0)),
    //      const_cast<uint16_t*>(&_array(4*quad+0,dnRow,0)),
    //      const_cast<uint16_t*>(&_array(4*quad+1,dnRow,0)),
    //      u, elemRowSize);

    MMCPY(const_cast<uint16_t*>(&_array(4*quad+2,upRow,0)), u, elemRowSize);
    MMCPY(const_cast<uint16_t*>(&_array(4*quad+3,upRow,0)), u, elemRowSize);
    MMCPY(const_cast<uint16_t*>(&_array(4*quad+2,dnRow,0)), u, elemRowSize);
    MMCPY(const_cast<uint16_t*>(&_array(4*quad+3,dnRow,0)), u, elemRowSize);
    MMCPY(const_cast<uint16_t*>(&_array(4*quad+0,upRow,0)), u, elemRowSize);
    MMCPY(const_cast<uint16_t*>(&_array(4*quad+1,upRow,0)), u, elemRowSize);
    MMCPY(const_cast<uint16_t*>(&_array(4*quad+0,dnRow,0)), u, elemRowSize);
    MMCPY(const_cast<uint16_t*>(&_array(4*quad+1,dnRow,0)), u, elemRowSize);
  }

  // Calibration rows
  const unsigned calibRows = _calib.shape()[1];
  for(unsigned i=0; i<calibRows; i++) {
    MMCPY(const_cast<uint16_t*>(&_calib(4*quad+2,i,0)), u, elemRowSize);
    MMCPY(const_cast<uint16_t*>(&_calib(4*quad+3,i,0)), u, elemRowSize);
    MMCPY(const_cast<uint16_t*>(&_calib(4*quad+0,i,0)), u, elemRowSize);
    MMCPY(const_cast<uint16_t*>(&_calib(4*quad+1,i,0)), u, elemRowSize);
  }

  // Environmental rows
  const unsigned envRows = _env.shape()[1];
  const uint32_t* u32 = reinterpret_cast<const uint32_t*>(u);
  for(unsigned i=0; i<envRows; i++) {
    MMCPY(const_cast<uint32_t*>(&_env(4*quad+2,i,0)), u32, elemRowSize/2);
    MMCPY(const_cast<uint32_t*>(&_env(4*quad+3,i,0)), u32, elemRowSize/2);
    MMCPY(const_cast<uint32_t*>(&_env(4*quad+0,i,0)), u32, elemRowSize/2);
    MMCPY(const_cast<uint32_t*>(&_env(4*quad+1,i,0)), u32, elemRowSize/2);
  }

  // Temperatures
#if 0
  const unsigned tempSize = 4;
  u = reinterpret_cast<const uint16_t*>(u32);
  MMCPY(const_cast<uint16_t*>(&_temp(4*quad+0)), u, tempSize);
  MMCPY(const_cast<uint16_t*>(&_temp(4*quad+2)), u, tempSize);
#endif
  return 1;
}

