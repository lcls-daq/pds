#include "pds/pvdaq/ControlsCameraServer.hh"
#include "pds/pvdaq/ConfigMonitor.hh"
#include "pds/pvdaq/CamPvServer.hh"
#include "pds/config/EpicsCamConfigType.hh"
#include "pds/config/EpicsCamDataType.hh"
#include "pds/config/FrameFexConfigType.hh"
#include "pds/config/PimImageConfigType.hh"
#include "pds/xtc/InDatagram.hh"
#include "pds/utility/Occurrence.hh"
#include "pds/service/GenericPool.hh"
#include <stdio.h>

static const unsigned EB_EVT_EXTRA = 0x10000;
static const unsigned NPRINT = 20;
static const unsigned PV_LEN = 64;

#define CREATE_IMG_PV(pv, name)            \
  snprintf(pvname, PV_LEN, name, pvbase);  \
  printf("Creating EpicsCA(%s)\n",pvname); \
  pv = new ImageServer(pvname,this);

#define CREATE_PV(pv, name)                       \
  snprintf(pvname, PV_LEN, name, pvbase);         \
  printf("Creating EpicsCA(%s)\n",pvname);        \
  pv = new ConfigServer(pvname, _configMonitor);  \
  _config_pvs.push_back(pv);

#define CHECK_PV(pv, val)                                         \
  if ((!pv->connected()) || (pv->fetch(&val, sizeof(val)) < 0)) { \
    printf("Error retrieving value of PV: %s\n", pv->name());     \
    error = true;                                                 \
  }

#define CHECK_STR_PV(pv, str, len)                              \
    if ((!pv->connected()) || (pv->fetch_str(str, len) < 0)) {  \
      printf("Error retrieving value of PV: %s\n", pv->name()); \
      str[0] = '\0';                                            \
      error = true;                                             \
    }

using namespace Pds::PvDaq;

ControlsCameraServer::ControlsCameraServer(const char*          pvbase,
                                           const Pds::DetInfo&  info,
                                           const unsigned       max_event_size,
                                           const unsigned       flags) :
  _pvbase     (pvbase),
  _image      (0),
  _nrows      (0),
  _ncols      (0),
  _nbits      (0),
  _gain       (0),
  _model      (0),
  _manufacturer(0),
  _exposure   (0),
  _xscale     (0),
  _yscale     (0),
  _xorg       (0),
  _xlen       (0),
  _yorg       (0),
  _ylen       (0),
  _width      (0),
  _height     (0),
  _depth      (0),
  _offset     (32),
  _roi_x_org  (0),
  _roi_x_len  (0),
  _roi_y_org  (0),
  _roi_y_len  (0),
  _exposure_val (0.0),
  _gain_val     (0.0),
  _xscale_val   (1.0),
  _yscale_val   (1.0),
  _manufacturer_str (new char[EpicsCamConfigType::DESC_CHAR_MAX]),
  _model_str        (new char[EpicsCamConfigType::DESC_CHAR_MAX]),
  _enabled    (false),
  _configured (false),
  _scale      (flags & (1<<SCALEPV)),
  _max_evt_sz (max_event_size),
  _frame_sz   (0),
  _configMonitor(new ConfigMonitor(*this)),
  _nprint     (NPRINT),
  _wrp        (0),
  _pool       (8)
{
  _xtc = Xtc(_epicsCamDataType, info);
  _manufacturer_str[0] = '\0';
  _model_str[0]        = '\0';

  //
  //  Create the pv connections
  //
  char pvname[PV_LEN];

  // image waveform
  CREATE_IMG_PV(_image, "%s:IMAGE1:ArrayData");
  // nrows config pv
  CREATE_PV(_nrows, "%s:IMAGE1:ArraySize1_RBV");
  // ncols config pv
  CREATE_PV(_ncols, "%s:IMAGE1:ArraySize0_RBV");
  // bits config pv
  CREATE_PV(_nbits, "%s:IMAGE1:BitsPerPixel_RBV");
  // gain config pv
  CREATE_PV(_gain, "%s:Gain_RBV");
  // model config pv
  CREATE_PV(_model, "%s:Model_RBV");
  // manufacturer config pv
  CREATE_PV(_manufacturer, "%s:Manufacturer_RBV");
  // exposure config pv
  CREATE_PV(_exposure, "%s:AcquireTime_RBV");
  // roi x begin config pv
  CREATE_PV(_xorg, "%s:MinX_RBV");
  // roi x len config pv
  CREATE_PV(_xlen, "%s:SizeX_RBV");
  // roi y begin config pv
  CREATE_PV(_yorg, "%s:MinY_RBV");
  // roi y len config pv
  CREATE_PV(_ylen, "%s:SizeY_RBV");

  if (_scale) {
    // xscale config pv
    CREATE_PV(_xscale, "%s:ScaleX_RBV");
    // yscale config pv
    CREATE_PV(_yscale, "%s:ScaleY_RBV");
  }

  for(unsigned i=0; i<_pool.size(); i++) {
    _pool[i] = new char[_max_evt_sz];
    reinterpret_cast<Dgram*>(_pool[i])->seq = Sequence(ClockTime(0,0),
                                                       TimeStamp(0,0,0,0));
  }
  printf("Initialized Epics Camera: %s\n", _pvbase);
}

ControlsCameraServer::~ControlsCameraServer()
{
  delete[] _manufacturer_str;
  delete[] _model_str;
  if (_image)
    delete _image;
  if (_nrows)
    delete _ncols;
  if (_nbits)
    delete _nbits;
  if (_gain)
    delete _gain;
  if (_model)
    delete _model;
  if (_manufacturer)
    delete _manufacturer;
  if (_exposure)
    delete _manufacturer;
  if (_xscale)
    delete _xscale;
  if (_yscale)
    delete _yscale;
  if (_xorg)
    delete _xorg;
  if (_xlen)
    delete _xlen;
  if (_yorg)
    delete _yorg;
  if (_ylen)
    delete _ylen;
  for(unsigned i=0; i<_pool.size(); i++) {
    if (_pool[i])
      delete[] _pool[i];
  }
}

int ControlsCameraServer::fill(char* payload, const void* p)
{
  const Dgram* dg = reinterpret_cast<const Dgram*>(p);
  if (_last > dg->seq.clock())
    return -1;

  _last = dg->seq.clock();

  _seq = dg->seq;
  _xtc = dg->xtc;

  memcpy(payload, &dg->xtc, dg->xtc.extent);
  return dg->xtc.extent;
}

void ControlsCameraServer::updated()
{
  if (!_enabled)
    return;

  Dgram* dg = new (_pool[_wrp]) Dgram;
  
  //
  //  Parse the raw data and fill the copyTo payload
  //
  unsigned fiducial = _image->nsec()&0x1ffff;

  dg->seq = Pds::Sequence(Pds::Sequence::Event,
                          Pds::TransitionId::L1Accept,
                          Pds::ClockTime(_image->sec(),_image->nsec()),
                          Pds::TimeStamp(0,fiducial,0));

  dg->env = _wrp;

  if (++_wrp==_pool.size()) _wrp=0;

  //
  //  Set the xtc header
  //
  dg->xtc = Xtc(_xtc.contains, _xtc.src);

  int len = sizeof(Xtc);

  do {
    // check that the needed pvs are connected
    bool lconnected=true;
    for(unsigned n=0; n<_config_pvs.size(); n++)
      lconnected &= _config_pvs[n]->connected();
    if (!lconnected) {
      if (_nprint) {
        printf("Some config PVs not connected\n");
        --_nprint;
      }
      dg->xtc.damage.increase(Pds::Damage::UserDefined);
      dg->xtc.damage.userBits(4);
      break;
    }

    EpicsCamDataType* frameptr = new (dg->xtc.alloc(sizeof(EpicsCamDataType)))
      EpicsCamDataType(_width, _height, _depth, _offset);
    char* dataptr = ((char*) frameptr) + sizeof(EpicsCamDataType);
    dg->xtc.alloc(frameptr->_sizeof()-sizeof(EpicsCamDataType));
    int fetch_sz = _image->fetch(dataptr, _frame_sz);
    if (fetch_sz < 0) {
      if (_nprint) {
        printf("Error fetching camera frame from %s\n", _image->name());
      }
      dg->xtc.damage.increase(Pds::Damage::UserDefined);
      dg->xtc.damage.userBits(2);
      break;
    } else if ((unsigned) fetch_sz != _frame_sz) {
      if (_nprint) {
        printf("Frame data does not match expected size: read %d, expected %lu\n", fetch_sz, _frame_sz);
        --_nprint;
      }
      dg->xtc.damage.increase(Pds::Damage::UserDefined);
      dg->xtc.damage.userBits(1);
      break;
    }
    len += frameptr->_sizeof();
  } while(0);

  dg->xtc.extent = len;

  post(dg);
}

Pds::Transition* ControlsCameraServer::fire(Pds::Transition* tr)
{
  switch (tr->id()) {
  case Pds::TransitionId::Configure:
    _nprint=NPRINT;
    _configured=true; break;
  case Pds::TransitionId::Unconfigure:
    _configured=false; break;
  case Pds::TransitionId::Enable:
    _enabled=true; break;
  case Pds::TransitionId::Disable:
    _enabled=false; break;
  case Pds::TransitionId::L1Accept:
  default:
    break;
  }
  return tr;
}

Pds::InDatagram* ControlsCameraServer::fire(Pds::InDatagram* dg)
{
  if (dg->seq.service()==Pds::TransitionId::Configure) {
    _last = ClockTime(0,0);
    _wrp  = 0;

    for(unsigned i=0; i<_pool.size(); i++)
      reinterpret_cast<Dgram*>(_pool[i])->seq = Sequence(ClockTime(0,0),
                                                         TimeStamp(0,0,0,0));
    bool error = false;
    printf("Retrieving camera configuration information from epics...\n");
    CHECK_PV(_ncols,    _width);
    CHECK_PV(_nrows,    _height);
    CHECK_PV(_nbits,    _depth);
    CHECK_PV(_gain,     _gain_val);
    CHECK_PV(_exposure, _exposure_val);
    CHECK_PV(_xorg,     _roi_x_org);
    CHECK_PV(_xlen,     _roi_x_len);
    CHECK_PV(_yorg,     _roi_y_org);
    CHECK_PV(_ylen,     _roi_y_len);
    if (_xscale) {
      CHECK_PV(_xscale, _xscale_val);
    }
    if (_yscale) {
      CHECK_PV(_yscale, _yscale_val);
    }

    CHECK_STR_PV(_model,        _model_str,         EpicsCamConfigType::DESC_CHAR_MAX);
    CHECK_STR_PV(_manufacturer, _manufacturer_str,  EpicsCamConfigType::DESC_CHAR_MAX);

    printf("Camera configuration information:\n"
           "  manufacturer:     %s\n"
           "  model:            %s\n"
           "  ncols:            %u\n"
           "  nrows:            %u\n"
           "  nbits:            %u\n"
           "  gain:             %f\n"
           "  exposure:         %f\n"
           "  roi (org, len):   x: (%u, %u), y: (%u, %u)\n",
           _manufacturer_str,
           _model_str,
           _width, _height, _depth,
           _gain_val, _exposure_val,
           _roi_x_org, _roi_x_len, _roi_y_org, _roi_y_len);
    if (_scale) {
      printf("  xscale:           %f\n"
             "  yscale:           %f\n",
             _xscale_val, _yscale_val);
    }

    Pds::Camera::FrameCoord roi_beg(_roi_x_org, _roi_x_org+_roi_x_len);
    Pds::Camera::FrameCoord roi_end(_roi_y_org, _roi_y_org+_roi_y_len);

    Pds::Xtc* xtc = new ((char*)dg->xtc.next())
      Pds::Xtc(_epicsCamConfigType, _xtc.src );
    new (xtc->alloc(sizeof(EpicsCamConfigType)))
      EpicsCamConfigType(_width, _height, _depth, EpicsCamConfigType::Mono, _exposure_val, _gain_val, _manufacturer_str, _model_str);
    dg->xtc.alloc(xtc->extent);

    xtc = new ((char*)dg->xtc.next())
      Pds::Xtc(_frameFexConfigType, _xtc.src);
    new (xtc->alloc(sizeof(FrameFexConfigType)))
      FrameFexConfigType(FrameFexConfigType::FullFrame, 1, FrameFexConfigType::NoProcessing, roi_beg, roi_end, 0, 0, 0);
    dg->xtc.alloc(xtc->extent);

    if (_scale) {
      xtc = new ((char*)dg->xtc.next())
        Pds::Xtc(_pimImageConfigType, _xtc.src );
      new (xtc->alloc(sizeof(PimImageConfigType)))
        PimImageConfigType(_xscale_val, _yscale_val);  
      dg->xtc.alloc(xtc->extent);
    }

    _frame_sz = _width * _height * ((_depth+7)/8);

    if (!error) {
      if (_frame_sz > _max_evt_sz) {
        printf("Error: EPICS_CA_MAX_ARRAY_BYTES not large enough for expected camera frame size %lu vs %u\n", _frame_sz, _max_evt_sz);
        error = true;
      } else if ((_frame_sz + EB_EVT_EXTRA) > _max_evt_sz) {
        printf("Error: expected event size will overrun eventbuilder buffer %lu vs. %u\n", (_frame_sz + EB_EVT_EXTRA), _max_evt_sz);
        error = true;
      }
    }

    if (error) {
      printf("Camera configuration failed!\n");
      char msg[128];
      snprintf(msg, 128, "ControlsCamera[%s] config error!", _pvbase);
      Pds::UserMessage* umsg = new (_occPool) Pds::UserMessage(msg);
      sendOcc(umsg);
      dg->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
    }
  }
  
  return dg;
}

void ControlsCameraServer::config_updated()
{
  if (_configured) {
    Pds::UserMessage* umsg = new (_occPool)Pds::UserMessage;
    umsg->append("Detected configuration change.  Forcing reconfiguration.");
    sendOcc(umsg);
    Occurrence* occ = new(_occPool) Occurrence(OccurrenceId::ClearReadout);
    sendOcc(occ);
  }
}

#undef CREATE_IMG_PV
#undef CREATE_PV
#undef CHECK_PV
#undef CHECK_STR_PV
