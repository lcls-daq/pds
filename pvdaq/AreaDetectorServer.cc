#include "pds/pvdaq/AreaDetectorServer.hh"
#include "pds/pvdaq/PvMacros.hh"
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

using namespace Pds::PvDaq;

const unsigned AreaDetectorServer::ENUM_PV_LEN = 64;

AreaDetectorServer::AreaDetectorServer(const char*          pvbase,
                                       const char*          pvimage,
                                       const Pds::DetInfo&  info,
                                       const unsigned       max_event_size,
                                       const unsigned       flags) :
  _pvbase       (pvbase),
  _pvimage      (pvimage ? pvimage : "IMAGE1"),
  _info         (info),
  _image        (NULL),
  _width        (0),
  _height       (0),
  _depth        (0),
  _bytes        (0),
  _offset       (0),
  _roi_x_org    (0),
  _roi_x_len    (0),
  _roi_x_end    (0),
  _roi_y_org    (0),
  _roi_y_len    (0),
  _roi_y_end    (0),
  _max_width    (0),
  _max_height   (0),
  _exposure     (0.0),
  _gain         (0.0),
  _xscale       (1.0),
  _yscale       (1.0),
  _manufacturer (new char[EpicsCamConfigType::DESC_CHAR_MAX]),
  _model        (new char[EpicsCamConfigType::DESC_CHAR_MAX]),
  _serial_num   (new char[EpicsCamConfigType::DESC_CHAR_MAX]),
  _firmware_ver (new char[EpicsCamConfigType::DESC_CHAR_MAX]),
  _image_pvname (new char[PV_LEN]),
  _enabled      (false),
  _configured   (false),
  _scale        (flags & (1<<SCALEPV)),
  _unixcam      (flags & (1<<UNIXCAM)),
  _max_evt_sz   (max_event_size),
  _frame_sz     (0),
  _configMonitor(new ConfigMonitor(*this)),
  _nprint       (NPRINT),
  _wrp          (0),
  _pool         (8),
  _context      (ca_current_context())
{
  _manufacturer[0] = '\0';
  _model[0]        = '\0';
  _serial_num[0]   = '\0';
  _firmware_ver[0] = '\0';

  //
  //  Create the pv connections
  //

  if (_unixcam) {
    // image waveform name
    create_pv_name(_image_pvname, PV_LEN, "LIVE_IMAGE_FAST", NULL);
    // nrows config pv
    CREATE_PV("N_OF_ROW", NULL, _height);
    // ncols config pv
    CREATE_PV("N_OF_COL", NULL, _width);
    // bits config pv
    CREATE_PV("N_OF_BITS", NULL, _depth);
    // gain config pv
    CREATE_PV("Gain", NULL, _gain);
    // model config pv
    CREATE_STR_PV("Model", NULL, _model, EpicsCamConfigType::DESC_CHAR_MAX);
    // manufacturer config pv
    CREATE_STR_PV("ID", NULL, _manufacturer, EpicsCamConfigType::DESC_CHAR_MAX);
    // exposure config pv
    // UNIX CAM HAS NONE!
    // roi x begin config pv
    CREATE_PV("StartROI_X", NULL, _roi_x_org);
    // roi x len config pv
    CREATE_PV("EndROI_X", NULL, _roi_x_end);
    // roi y begin config pv
    CREATE_PV("StartROI_Y", NULL, _roi_y_org);
    // roi y len config pv
    CREATE_PV("EndROI_Y", NULL, _roi_y_end);

    // Disable scale - not supported for UNIXCAM
    if (_scale) {
      printf("Warning: scaling PVs requested but this is not supported for UNIXCAM!\n");
      _scale = false;
    }
  } else {
    // image waveform name
    create_pv_name(_image_pvname, PV_LEN, "ArrayData", _pvimage);
    // nrows config pv
    CREATE_PV("ArraySize1_RBV", _pvimage, _height);
    // ncols config pv
    CREATE_PV("ArraySize0_RBV", _pvimage, _width);
    // bits config pv
    CREATE_PV("BitsPerPixel_RBV", _pvimage, _depth);
    // gain config pv
    CREATE_PV("Gain_RBV", NULL, _gain);
    // model config pv
    CREATE_STR_PV("Model_RBV", NULL, _model, EpicsCamConfigType::DESC_CHAR_MAX);
    // manufacturer config pv
    CREATE_STR_PV("Manufacturer_RBV", NULL, _manufacturer, EpicsCamConfigType::DESC_CHAR_MAX);
    // serial number config pv
    CREATE_STR_PV("SerialNumber_RBV", NULL, _serial_num, EpicsCamConfigType::DESC_CHAR_MAX);
    // firmware version config pv
    CREATE_STR_PV("FirmwareVersion_RBV", NULL, _firmware_ver, EpicsCamConfigType::DESC_CHAR_MAX);
    // exposure config pv
    CREATE_PV("AcquireTime_RBV", NULL, _exposure);
    // roi x begin config pv
    CREATE_PV("MinX_RBV", NULL, _roi_x_org);
    // roi x len config pv
    CREATE_PV("SizeX_RBV", NULL, _roi_x_len);
    // max image width
    CREATE_PV("MaxSizeX_RBV", NULL, _max_width);
    // roi y begin config pv
    CREATE_PV("MinY_RBV", NULL, _roi_y_org);
    // roi y len config pv
    CREATE_PV("SizeY_RBV", NULL, _roi_y_len);
    // max image height
    CREATE_PV("MaxSizeY_RBV", NULL, _max_height);

    if (_scale) {
      // xscale config pv
      CREATE_PV("ScaleX_RBV", NULL, _xscale);
      // yscale config pv
      CREATE_PV("ScaleY_RBV", NULL, _yscale);
    }
  }

  for(unsigned i=0; i<_pool.size(); i++) {
    _pool[i] = new char[_max_evt_sz];
    reinterpret_cast<Dgram*>(_pool[i])->seq = Sequence(ClockTime(0,0),
                                                       TimeStamp(0,0,0,0));
  }
  printf("Initialized Epics Camera: %s\n", _pvbase);
}

AreaDetectorServer::~AreaDetectorServer()
{
  delete[] _manufacturer;
  delete[] _model;
  delete[] _serial_num;
  delete[] _firmware_ver;
  delete[] _image_pvname;
  if (_image)
    delete _image;
  for (size_t n=0; n<_config_pvs.size(); n++) {
    delete _config_pvs[n];
  }
  _config_pvs.clear();
  for(unsigned i=0; i<_pool.size(); i++) {
    if (_pool[i])
      delete[] _pool[i];
  }
}

int AreaDetectorServer::fill(char* payload, const void* p)
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

void AreaDetectorServer::updated()
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

    char* dataptr = alloc_data_type(dg->xtc);
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
  } while(0);

  post(dg);
}

Pds::Transition* AreaDetectorServer::fire(Pds::Transition* tr)
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

Pds::InDatagram* AreaDetectorServer::fire(Pds::InDatagram* dg)
{
  if (dg->seq.service()==Pds::TransitionId::Configure) {
    _last = ClockTime(0,0);
    _wrp  = 0;

    for(unsigned i=0; i<_pool.size(); i++)
      reinterpret_cast<Dgram*>(_pool[i])->seq = Sequence(ClockTime(0,0),
                                                         TimeStamp(0,0,0,0));
    bool error = false;
    printf("Retrieving camera configuration information from epics...\n");
    for (size_t n=0; n<_config_pvs.size(); n++) {
      if (_config_pvs[n]->check_fetch()) {
        printf("Error retrieving value of PV: %s\n", _config_pvs[n]->name());
        error = true;
      }
    }

    if (_unixcam) {
      // Unixcam directly gives the end coordinate of the ROI
      _roi_x_len = _roi_x_end - _roi_x_org;
      _roi_y_len = _roi_y_end - _roi_y_org;
    } else {
      // Areadet gives the len of the ROI
      _roi_x_end = _roi_x_org + _roi_x_len;
      _roi_y_end = _roi_y_org + _roi_y_len;
    }

    if (!_image) {
      if (ca_current_context() == NULL) ca_attach_context(_context);
      printf("Creating EpicsCA(%s)\n", _image_pvname);
      _image = new ImageServer(_image_pvname, this, _width * _height);

      // try waiting for pv to connect to read element size
      int tries = 100;
      while(!_image->connected() && tries > 0) {
        usleep(10000);
        tries--;
      }
      if (_image->connected()) {
        _bytes = _image->elem_size();
      } else {
        printf("Error: failed to read image element size from image pv: %s\n", _image_pvname);
        _bytes = 0;
        error = true;
      }
    }

    // run the camera specific configuration if any
    if (configure()) {
      printf("Error: failed camera specific configuration\n");
      error = true;
    }

    printf("Camera configuration information:\n"
           "  manufacturer:     %s\n"
           "  model:            %s\n"
           "  serial number:    %s\n"
           "  firmware:         %s\n"
           "  ncols:            %u\n"
           "  nrows:            %u\n"
           "  nbits:            %u\n"
           "  bytes per pixel:  %u\n"
           "  gain:             %f\n"
           "  exposure:         %f\n"
           "  roi (org, end):   x: (%u, %u), y: (%u, %u)\n",
           _manufacturer, _model, _serial_num, _firmware_ver,
           _width, _height, _depth, _bytes,
           _gain, _exposure,
           _roi_x_org, _roi_x_end, _roi_y_org, _roi_y_end);
    if (_scale) {
      printf("  xscale:           %f\n"
             "  yscale:           %f\n",
             _xscale, _yscale);
    }
    // print camera specific configuration
    show_configuration();

    Pds::Camera::FrameCoord roi_beg(_roi_x_org, _roi_x_end);
    Pds::Camera::FrameCoord roi_end(_roi_y_org, _roi_y_end);

    Pds::Xtc* xtc = alloc_config_type(dg);

    xtc = new ((char*)dg->xtc.next())
      Pds::Xtc(_frameFexConfigType, _xtc.src);
    new (xtc->alloc(sizeof(FrameFexConfigType)))
      FrameFexConfigType(FrameFexConfigType::FullFrame, 1, FrameFexConfigType::NoProcessing, roi_beg, roi_end, 0, 0, 0);
    dg->xtc.alloc(xtc->extent);

    if (_scale) {
      xtc = new ((char*)dg->xtc.next())
        Pds::Xtc(_pimImageConfigType, _xtc.src );
      new (xtc->alloc(sizeof(PimImageConfigType)))
        PimImageConfigType(_xscale, _yscale);
      dg->xtc.alloc(xtc->extent);
    }

    _frame_sz = _width * _height * std::max((_depth+7)/8, _bytes);

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
  } else if (dg->seq.service()==Pds::TransitionId::Unconfigure) {
    if (_image) {
      if (ca_current_context() == NULL) ca_attach_context(_context);
      delete _image;
      _image = 0;
    }
  }

  return dg;

}

void AreaDetectorServer::config_updated()
{
  if (_configured) {
    Pds::UserMessage* umsg = new (_occPool)Pds::UserMessage;
    umsg->append("Detected configuration change.  Forcing reconfiguration.");
    sendOcc(umsg);
    Occurrence* occ = new(_occPool) Occurrence(OccurrenceId::ClearReadout);
    sendOcc(occ);
    _configured = false; // inhibit more of these
  }
}

void AreaDetectorServer::create_pv_name(char* pvname, size_t len, const char* name, const char* prefix)
{
  if (prefix) {
    snprintf(pvname, len, "%s:%s:%s", _pvbase, prefix, name);
  } else {
    snprintf(pvname, len, "%s:%s", _pvbase, name);
  }
}

void AreaDetectorServer::create_pv(const char* name, const char* prefix, bool is_enum, void* copyTo, size_t len, ConfigServer::Type type)
{
  char pvname[PV_LEN];
  create_pv_name(pvname, sizeof(pvname), name, prefix);
  printf("Creating EpicsCA(%s)\n", pvname);
  ConfigServer* pv = new ConfigServer(pvname, _configMonitor, is_enum, copyTo, len, type);
  _config_pvs.push_back(pv);
}

EpicsCamServer::EpicsCamServer(const char*          pvbase,
                               const char*          pvimage,
                               const Pds::DetInfo&  info,
                               const unsigned       max_event_size,
                               const unsigned       flags) :
  AreaDetectorServer(pvbase, pvimage, info, max_event_size, flags)
{
  _xtc = Xtc(_epicsCamDataType, info);
}

EpicsCamServer::~EpicsCamServer()
{}

bool EpicsCamServer::configure()
{
  return false;
}

void EpicsCamServer::show_configuration()
{}

char* EpicsCamServer::alloc_data_type(Xtc& xtc)
{
  uint32_t depth = (_bytes > (_depth+7)/8) ? _bytes*8 : _depth;
  EpicsCamDataType* frameptr = new (xtc.alloc(sizeof(EpicsCamDataType)))
    EpicsCamDataType(_width, _height, depth, _offset);
  xtc.alloc(frameptr->_sizeof()-sizeof(EpicsCamDataType));
  char* dataptr = ((char*) frameptr) + sizeof(EpicsCamDataType);
  return dataptr;
}
