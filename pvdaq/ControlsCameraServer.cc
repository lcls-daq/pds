#include "pds/pvdaq/ControlsCameraServer.hh"
#include "pds/pvdaq/ConfigMonitor.hh"
#include "pds/pvdaq/CamPvServer.hh"
#include "pds/config/RayonixConfigType.hh"
#include "pds/config/StreakConfigType.hh"
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
static const unsigned STRK_FPATH_LEN = 10000;

#define CREATE_PV_NAME(pvname, fmt, ...)                \
  snprintf(pvname, PV_LEN, fmt, pvbase, ##__VA_ARGS__);

#define CREATE_PV(pv, fmt, ...)                         \
  snprintf(pvname, PV_LEN, fmt, pvbase, ##__VA_ARGS__); \
  printf("Creating EpicsCA(%s)\n",pvname);              \
  pv = new ConfigServer(pvname, _configMonitor);        \
  _config_pvs.push_back(pv);

#define CREATE_ENUM_PV(pv, fmt, ...)                    \
  snprintf(pvname, PV_LEN, fmt, pvbase, ##__VA_ARGS__); \
  printf("Creating EpicsCA(%s)\n",pvname);              \
  pv = new ConfigServer(pvname, _configMonitor, true);  \
  _config_pvs.push_back(pv);

#define CHECK_PV(pv, val)                                         \
  if ((!pv->connected()) || (pv->fetch(&val, sizeof(val)) < 0)) { \
    printf("Error retrieving value of PV: %s\n", pv->name());     \
    error = true;                                                 \
  }

#define CHECK_LONG_STR_PV(pv, str, len)                       \
  if ((!pv->connected()) || (pv->fetch(str, len) < 0)) {      \
    printf("Error retrieving value of PV: %s\n", pv->name()); \
    str[0] = '\0';                                            \
    error = true;                                             \
  }

#define CHECK_STR_PV(pv, str, len)                            \
  if ((!pv->connected()) || (pv->fetch_str(str, len) < 0)) {  \
    printf("Error retrieving value of PV: %s\n", pv->name()); \
    str[0] = '\0';                                            \
    error = true;                                             \
  }

using namespace Pds::PvDaq;

ControlsCameraServer::ControlsCameraServer(const char*          pvbase,
                                           const char*          pvimage,
                                           const Pds::DetInfo&  info,
                                           const unsigned       max_event_size,
                                           const unsigned       flags) :
  _pvbase     (pvbase),
  _pvimage    (pvimage ? pvimage : "IMAGE1"),
  _info       (info),
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
  _rnx_bin    (0),
  _rnx_trig   (0),
  _rnx_mode   (0),
  _strk_time  (0),
  _strk_mode  (0),
  _strk_gate  (0),
  _strk_shut  (0),
  _strk_trig  (0),
  _strk_fto   (0),
  _strk_path  (0),
  _rnx_bin_val(0),
  _rnx_trig_val(0),
  _rnx_mode_val(0),
  _strk_fto_val(0),
  _width      (0),
  _height     (0),
  _depth      (0),
  _bytes      (0),
  _offset     (0),
  _roi_x_org  (0),
  _roi_x_len  (0),
  _roi_x_end  (0),
  _roi_y_org  (0),
  _roi_y_len  (0),
  _roi_y_end  (0),
  _exposure_val (0.0),
  _gain_val     (0.0),
  _xscale_val   (1.0),
  _yscale_val   (1.0),
  _manufacturer_str (new char[EpicsCamConfigType::DESC_CHAR_MAX]),
  _model_str        (new char[EpicsCamConfigType::DESC_CHAR_MAX]),
  _image_pvname     (new char[PV_LEN]),
  _strk_time_str    (new char[PV_LEN]),
  _strk_mode_str    (new char[PV_LEN]),
  _strk_gate_str    (new char[PV_LEN]),
  _strk_shut_str    (new char[PV_LEN]),
  _strk_trig_str    (new char[PV_LEN]),
  _strk_path_str    (new char[STRK_FPATH_LEN]),
  _enabled    (false),
  _configured (false),
  _scale      (flags & (1<<SCALEPV)),
  _unixcam    (flags & (1<<UNIXCAM)),
  _max_evt_sz (max_event_size),
  _frame_sz   (0),
  _configMonitor(new ConfigMonitor(*this)),
  _nprint     (NPRINT),
  _wrp        (0),
  _pool       (8),
  _context    (ca_current_context())
{
  _xtc = Xtc(_epicsCamDataType, info);
  _manufacturer_str[0] = '\0';
  _model_str[0]        = '\0';

  //
  //  Create the pv connections
  //
  char pvname[PV_LEN];

  if (_unixcam) {
    // image waveform name
    CREATE_PV_NAME(_image_pvname, "%s:LIVE_IMAGE_FAST")
    // nrows config pv
    CREATE_PV(_nrows, "%s:N_OF_ROW");
    // ncols config pv
    CREATE_PV(_ncols, "%s:N_OF_COL");
    // bits config pv
    CREATE_PV(_nbits, "%s:N_OF_BITS");
    // gain config pv
    CREATE_PV(_gain, "%s:Gain");
    // model config pv
    CREATE_PV(_model, "%s:Model");
    // manufacturer config pv
    CREATE_PV(_manufacturer, "%s:ID");
    // exposure config pv
    // UNIX CAM HAS NONE!
    // roi x begin config pv
    CREATE_PV(_xorg, "%s:StartROI_X");
    // roi x len config pv
    CREATE_PV(_xlen, "%s:EndROI_X");
    // roi y begin config pv
    CREATE_PV(_yorg, "%s:StartROI_Y");
    // roi y len config pv
    CREATE_PV(_ylen, "%s:EndROI_Y");

    // Disable scale - not supported for UNIXCAM
    if (_scale) {
      printf("Warning: scaling PVs requested but this is not supported for UNIXCAM!\n");
      _scale = false;
    }
  } else {
    // image waveform name
    CREATE_PV_NAME(_image_pvname, "%s:%s:ArrayData", _pvimage);
    // nrows config pv
    CREATE_PV(_nrows, "%s:%s:ArraySize1_RBV", _pvimage);
    // ncols config pv
    CREATE_PV(_ncols, "%s:%s:ArraySize0_RBV", _pvimage);
    // bits config pv
    CREATE_PV(_nbits, "%s:%s:BitsPerPixel_RBV", _pvimage);
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

    if (info.device() == Pds::DetInfo::Rayonix) {
      // Rayonix binning pv
      CREATE_PV(_rnx_bin, "%s:Bin_RBV");
      // Rayonix trigger pv
      CREATE_PV(_rnx_trig, "%s:TriggerMode");
      // Rayonix readout mode pv
      CREATE_PV(_rnx_mode, "%s:ReadoutMode");
    } else if (info.device() == Pds::DetInfo::StreakC7700) {
      // Streak camera time range pv
      CREATE_ENUM_PV(_strk_time, "%s:TimeRange_RBV");
      // Streak camera mode pv
      CREATE_ENUM_PV(_strk_mode, "%s:TriggerMode_RBV");
      // Streak camera gate mode pv
      CREATE_ENUM_PV(_strk_gate, "%s:GateMode_RBV");
      // Streak camera shutter mode pv
      CREATE_ENUM_PV(_strk_shut, "%s:Shutter_RBV");
      // Streak camera tigger mode pv
      CREATE_ENUM_PV(_strk_trig, "%s:ImageMode_RBV");
      // Streak camera focus time over pv
      CREATE_PV(_strk_fto, "%s:FocusTimeOver_RBV");
      // Streak camera configuration filepath pv
      CREATE_PV(_strk_path, "%s:ScalingFilePath");
    }

    if (_scale) {
      // xscale config pv
      CREATE_PV(_xscale, "%s:ScaleX_RBV");
      // yscale config pv
      CREATE_PV(_yscale, "%s:ScaleY_RBV");
    }
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
  delete[] _image_pvname;
  delete[] _strk_time_str;
  delete[] _strk_mode_str;
  delete[] _strk_path_str;
  delete[] _strk_gate_str;
  delete[] _strk_shut_str;
  delete[] _strk_trig_str;
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
  if (_rnx_bin)
    delete _rnx_bin;
  if (_rnx_trig)
    delete _rnx_trig;
  if (_rnx_mode)
    delete _rnx_mode;
  if (_strk_time)
    delete _strk_time;
  if (_strk_mode)
    delete _strk_mode;
  if (_strk_gate)
    delete _strk_gate;
  if (_strk_shut)
    delete _strk_shut;
  if (_strk_trig)
    delete _strk_trig;
  if (_strk_fto)
    delete _strk_fto;
  if (_strk_path)
    delete _strk_path;
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

    uint32_t depth = (_bytes > (_depth+7)/8) ? _bytes*8 : _depth;
    EpicsCamDataType* frameptr = new (dg->xtc.alloc(sizeof(EpicsCamDataType)))
      EpicsCamDataType(_width, _height, depth, _offset);
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
    uint32_t rnx_trig_ddl = 0;
    uint64_t strk_time_ddl = 0;
    double calib[StreakConfigType::NumCalibConstants];
    memset(calib, 0, sizeof(calib));
    RayonixConfigType::ReadoutMode rnx_mode_ddl = RayonixConfigType::Unknown;
    StreakConfigType::DeviceMode strk_mode_ddl = StreakConfigType::Focus;
    StreakConfigType::GateMode strk_gate_ddl = StreakConfigType::Normal;
    StreakConfigType::ShutterMode strk_shut_ddl = StreakConfigType::Closed;
    StreakConfigType::TriggerMode strk_trig_ddl = StreakConfigType::Single;
    StreakConfigType::CalibScale strk_scale_ddl = StreakConfigType::Seconds;
    printf("Retrieving camera configuration information from epics...\n");
    CHECK_PV(_ncols,    _width);
    CHECK_PV(_nrows,    _height);
    CHECK_PV(_nbits,    _depth);
    CHECK_PV(_gain,     _gain_val);
    if (_exposure) { // Not all cameras have this!
      CHECK_PV(_exposure, _exposure_val);
    }
    CHECK_PV(_xorg,     _roi_x_org);
    CHECK_PV(_xlen,     _roi_x_len);
    CHECK_PV(_yorg,     _roi_y_org);
    CHECK_PV(_ylen,     _roi_y_len);
    if (_xscale) { // Not all cameras have this!
      CHECK_PV(_xscale, _xscale_val);
    }
    if (_yscale) { // Not all cameras have this!
      CHECK_PV(_yscale, _yscale_val);
    }
    if (_rnx_bin) { // Not all cameras have this!
      CHECK_PV(_rnx_bin, _rnx_bin_val);
    }
    if (_rnx_trig) { // Not all cameras have this!
      CHECK_PV(_rnx_trig, _rnx_trig_val);
      // Convert to DDL value scheme
      switch (_rnx_trig_val) {
        case 1:
          rnx_trig_ddl = 0;
          break;
        case 2:
          rnx_trig_ddl = 1;
          break;
        case 3:
          rnx_trig_ddl = 2;
          break;
        default:
          rnx_trig_ddl = 3;
      }
    }
    if (_rnx_mode) { // Not all cameras have this!
      CHECK_PV(_rnx_mode, _rnx_mode_val);
      // Convert to DDL enum scheme
      switch (_rnx_mode_val) {
        case 0:
          rnx_mode_ddl = RayonixConfigType::Standard;
          break;
        case 1:
          rnx_mode_ddl = RayonixConfigType::LowNoise;
          break;
      }
    }
    if (_strk_time) { // Not all cameras have this!
      CHECK_STR_PV(_strk_time, _strk_time_str, EpicsCamConfigType::DESC_CHAR_MAX);
      char* unit;
      double time_val = strtod(_strk_time_str, &unit);
      if (unit) {
        unit++;
        if (!strcmp(unit, "ns")) {
          strk_time_ddl = (uint64_t) (time_val * 1e3);
        } else if (!strcmp(unit, "us")) {
          strk_time_ddl = (uint64_t) (time_val * 1e6);
        } else if (!strcmp(unit, "ms")) {
          strk_time_ddl = (uint64_t) (time_val * 1e9);
        } else if (!strcmp(unit, "s")) {
          strk_time_ddl = (uint64_t) (time_val * 1e12);
        } else {
          printf("Error: failed to parse streak camera time range units: %s\n", _strk_time_str);
          error = true;
        }
      } else {
        printf("Error: failed to parse streak camera time range: %s\n", _strk_time_str);
        error = true;
      }
    }
    if (_strk_mode) { // Not all cameras have this!
      CHECK_STR_PV(_strk_mode, _strk_mode_str, EpicsCamConfigType::DESC_CHAR_MAX);
      // Convert to DDL value scheme
      if (!strcmp(_strk_mode_str, "Focus")) {
        strk_mode_ddl = StreakConfigType::Focus;
      } else if (!strcmp(_strk_mode_str, "Operate")) {
        strk_mode_ddl = StreakConfigType::Operate;
      } else {
        printf("Error: failed to parse streak camera mode enum value: %s\n", _strk_mode_str);
        error = true;
      }
    }
    if (_strk_gate) { // Not all cameras have this!
      CHECK_STR_PV(_strk_gate, _strk_gate_str, EpicsCamConfigType::DESC_CHAR_MAX);
      // Convert to DDL value scheme
      if (!strcmp(_strk_gate_str, "Normal")) {
        strk_gate_ddl = StreakConfigType::Normal;
      } else if (!strcmp(_strk_gate_str, "Gate")) {
        strk_gate_ddl = StreakConfigType::Gate;
      } else if (!strcmp(_strk_gate_str, "Open Fixed")) {
        strk_gate_ddl = StreakConfigType::OpenFixed;
      } else {
        printf("Error: failed to parse streak camera gate enum value: %s\n", _strk_gate_str);
        error = true;
      }
    }
    if (_strk_shut) { // Not all cameras have this!
      CHECK_STR_PV(_strk_shut, _strk_shut_str, EpicsCamConfigType::DESC_CHAR_MAX);
      // Convert to DDL value scheme
      if (!strcmp(_strk_shut_str, "Closed")) {
        strk_shut_ddl = StreakConfigType::Closed;
      } else if (!strcmp(_strk_shut_str, "Open")) {
        strk_shut_ddl = StreakConfigType::Open;
      } else {
        printf("Error: failed to parse streak camera shutter enum value: %s\n", _strk_shut_str);
        error = true;
      }
    }
    if (_strk_trig) { // Not all cameras have this!
      CHECK_STR_PV(_strk_trig, _strk_trig_str, EpicsCamConfigType::DESC_CHAR_MAX);
      // Convert to DDL value scheme
      if (!strcmp(_strk_trig_str, "Single")) {
        strk_trig_ddl = StreakConfigType::Single;
      } else if (!strcmp(_strk_trig_str, "Continuous")) {
        strk_trig_ddl = StreakConfigType::Continuous;
      } else {
        printf("Error: failed to parse streak camera trigger enum value: %s\n", _strk_shut_str);
        error = true;
      }
    }
    if (_strk_fto) { // Not all cameras have this!
      CHECK_PV(_strk_fto, _strk_fto_val);
    }
    if (_strk_path) { // Not all cameras have this!
      CHECK_LONG_STR_PV(_strk_path, _strk_path_str, STRK_FPATH_LEN);
      FILE* f = fopen(_strk_path_str, "r");
      if (f) {
        bool failed = true;
        int skip = 3;
        size_t line_sz = 1024;
        char* line = (char *)malloc(line_sz);

        while (getline(&line, &line_sz, f) != -1) {
          if (skip) {
            skip--;
          } else {
            char* token = strtok(line, ",");
            if (token && !strcmp(token, _strk_time_str)) {
              int pos = 0;
              token = strtok(NULL, ",");
              while (token != NULL) {
                if (pos == 0) {
                  if (!strcmp(token, "ns"))
                    strk_scale_ddl = StreakConfigType::Nanoseconds;
                  else if (!strcmp(token, "us"))
                    strk_scale_ddl = StreakConfigType::Microseconds;
                  else if (!strcmp(token, "ms"))
                    strk_scale_ddl = StreakConfigType::Milliseconds;
                  else if (!strcmp(token, "s"))
                    strk_scale_ddl = StreakConfigType::Seconds;
                } else if((pos > 1) && (pos < (StreakConfigType::NumCalibConstants + 2))) {
                  calib[pos - 2] = strtod(token, NULL);
                }
                token = strtok(NULL, ",");
                pos++;
              }
              failed = (pos != (StreakConfigType::NumCalibConstants + 2));
            }
          }
        }

        if (failed) {
          printf("Error: failed to parse streak camera calibration file: %s\n", _strk_path_str);
          error = true;
        }

        if (line) free(line);
      } else {
        printf("Error: failed to open streak camera calibration file: %s\n", _strk_path_str);
        error = true;
      }
    }

    CHECK_STR_PV(_model,        _model_str,         EpicsCamConfigType::DESC_CHAR_MAX);
    CHECK_STR_PV(_manufacturer, _manufacturer_str,  EpicsCamConfigType::DESC_CHAR_MAX);

    if (_unixcam) {
      // Unixcam directly gives the end coordinate of the ROI
      _roi_x_end = _roi_x_len;
      _roi_y_end = _roi_y_len;
    } else {
      // Areadet gives the len of the ROI
      _roi_x_end = _roi_x_org+_roi_x_len;
      _roi_y_end = _roi_y_org+_roi_y_len;
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

    printf("Camera configuration information:\n"
           "  manufacturer:     %s\n"
           "  model:            %s\n"
           "  ncols:            %u\n"
           "  nrows:            %u\n"
           "  nbits:            %u\n"
           "  bytes per pixel:  %u\n"
           "  gain:             %f\n"
           "  exposure:         %f\n"
           "  roi (org, end):   x: (%u, %u), y: (%u, %u)\n",
           _manufacturer_str,
           _model_str,
           _width, _height, _depth, _bytes,
           _gain_val, _exposure_val,
           _roi_x_org, _roi_x_end, _roi_y_org, _roi_y_end);
    if (_scale) {
      printf("  xscale:           %f\n"
             "  yscale:           %f\n",
             _xscale_val, _yscale_val);
    }
    if (_info.device() == Pds::DetInfo::StreakC7700) {
      printf("  time range:       %s\n"
             "  calib scale:      %d\n"
             "  calib constants: ",
             _strk_time_str,
             strk_scale_ddl);
      for (int i=0; i<StreakConfigType::NumCalibConstants; i++) {
        printf(" %g", calib[i]);
      }
      printf("\n");
    }

    Pds::Camera::FrameCoord roi_beg(_roi_x_org, _roi_x_end);
    Pds::Camera::FrameCoord roi_end(_roi_y_org, _roi_y_end);

    Pds::Xtc* xtc;

    if (_info.device() == Pds::DetInfo::Rayonix) {
      xtc = new ((char*)dg->xtc.next())
        Pds::Xtc(_rayonixConfigType, _xtc.src );
      new (xtc->alloc(sizeof(RayonixConfigType)))
        RayonixConfigType((uint8_t) _rnx_bin_val, (uint8_t) _rnx_bin_val, 0, (uint32_t) _exposure_val, rnx_trig_ddl, 0, 0, rnx_mode_ddl, _model_str);
      dg->xtc.alloc(xtc->extent);
    } else if (_info.device() == Pds::DetInfo::StreakC7700) {
      xtc = new ((char*)dg->xtc.next())
        Pds::Xtc(_streakConfigType, _xtc.src );
      new (xtc->alloc(sizeof(StreakConfigType)))
        StreakConfigType(strk_time_ddl, strk_mode_ddl, strk_gate_ddl, (uint32_t) _gain_val, strk_shut_ddl, strk_trig_ddl, _strk_fto_val, _exposure_val, strk_scale_ddl, calib);
      dg->xtc.alloc(xtc->extent);
    } else {
      xtc = new ((char*)dg->xtc.next())
        Pds::Xtc(_epicsCamConfigType, _xtc.src );
      new (xtc->alloc(sizeof(EpicsCamConfigType)))
        EpicsCamConfigType(_width, _height, _depth, EpicsCamConfigType::Mono, _exposure_val, _gain_val, _manufacturer_str, _model_str);
      dg->xtc.alloc(xtc->extent);
    }

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

void ControlsCameraServer::config_updated()
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

#undef CREATE_PV_NAME
#undef CREATE_PV
#undef CREATE_ENUM_PV
#undef CHECK_PV
#undef CHECK_LONG_STR_PV
#undef CHECK_STR_PV
