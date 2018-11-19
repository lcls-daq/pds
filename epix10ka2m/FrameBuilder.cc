#include "pds/epix10ka2m/FrameBuilder.hh"
#include "pds/pgp/DataImportFrame.hh"
#include "pds/pgp/Pgp.hh"
#include "pds/xtc/Datagram.hh"
#include "pds/xtc/XtcType.hh"
#include "pdsdata/xtc/Xtc.hh"

//#define SHORT_PAYLOAD
//#define DBUG

using namespace Pds::Epix10ka2m;

FrameBuilder::FrameBuilder(const Datagram&             in, 
                           Datagram&                   out, 
                           const Epix10ka2MConfigType& cfg) :
  Pds::XtcIterator(const_cast<Xtc*>(&in.xtc)),
  _qmask(0)
{
  memset(_qxtc, 0, 4*sizeof(Xtc*));

  _next = out.xtc.next();

  //  Cache pointers to the quad(s) and copy the remainder (advancing _next)
  iterate();

  if ((_qmask&0xf)==0xf) {
    const Xtc* inx = &in.xtc + 1;
    Xtc* xtc = new ((char*)_next) Xtc(_epix10kaDataArray, inx->src);

    _payload = new (xtc->next()) Epix10kaDataArray;
    _array   = _payload->frame            (cfg);
    _calib   = _payload->calibrationRows  (cfg);
    _env     = _payload->environmentalRows(cfg);
    //  _temp   = _payload->temperatures     (cfg);

#if 1
    for(unsigned i=0; i<4; i++)
      _add_quad(i);
#endif

    xtc->alloc(Epix10kaDataArray::_sizeof(cfg));
    _next = xtc->next();
  }

#ifdef SHORT_PAYLOAD
#else
  out.xtc.alloc( reinterpret_cast<char*>(_next) - 
                 reinterpret_cast<char*>(&out.xtc+1));
#endif
}

FrameBuilder::FrameBuilder(const Datagram&               in, 
                           Datagram&                     out, 
                           const Epix10kaQuadConfigType& cfg) :
  Pds::XtcIterator(const_cast<Xtc*>(&in.xtc)),
  _qmask(0)
{
  _next = out.xtc.next();

  //  Cache pointers to the quad(s) and copy the remainder (advancing _next)
  iterate();

  if ((_qmask&0x1)==0x1) {
    const Xtc* inx = &in.xtc + 1;
    Xtc* xtc = new ((char*)_next) Xtc(_epix10kaDataArray, inx->src);

    _payload = new (xtc->next()) Epix10kaDataArray;
    _array   = _payload->frame            (cfg);
    _calib   = _payload->calibrationRows  (cfg);
    _env     = _payload->environmentalRows(cfg);
    //  _temp   = _payload->temperatures     (cfg);

    _add_quad(0);

    xtc->alloc(Epix10kaDataArray::_sizeof(cfg));
    _next = xtc->next();
  }

  out.xtc.alloc( reinterpret_cast<char*>(_next) - 
                 reinterpret_cast<char*>(&out.xtc+1));
}

int FrameBuilder::process(Xtc* xtc)
{
#ifdef DBUG
  { const unsigned* p = reinterpret_cast<const unsigned*>(xtc);
    printf("FB::process %08x.%08x.%08x.%08x.%08x\n",p[0],p[1],p[2],p[3],p[4]); }
#endif

  if (xtc->contains.id() == _xtcType.id()) {
    iterate(xtc);
    return 1;
  }

  if (xtc->contains.value() != _epix10kaDataType.value()) {
    memcpy((char*)_next, xtc, xtc->extent);
    _next = _next->next();
    return 1;
  }

  //  Set the frame number
  const Pds::Pgp::DataImportFrame* e = reinterpret_cast<Pds::Pgp::DataImportFrame*>(xtc->payload());
  unsigned quad = e->lane()-Pds::Pgp::Pgp::portOffset();
  _qmask |= (1<<quad);
  _qxtc[quad] = xtc;

  return 1;
}

void FrameBuilder::_add_quad(unsigned quad)
{
  const Xtc* xtc = _qxtc[quad];
  const Pds::Pgp::DataImportFrame* e = reinterpret_cast<Pds::Pgp::DataImportFrame*>(xtc->payload());

  //  Each quad arrives on a separate lane
  //  A super row crosses 2 elements; each element contains 2x2 ASICs
  const unsigned asicRows     = Pds::Epix::Config10ka::_numberOfRowsPerAsic;
  const unsigned elemRowSize  = Pds::Epix::Config10ka::_numberOfAsicsPerRow*Pds::Epix::Config10ka::_numberOfPixelsPerAsicRow;

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
}

