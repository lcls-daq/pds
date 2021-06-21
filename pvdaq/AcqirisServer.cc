#include "pds/pvdaq/AcqirisServer.hh"
#include "pds/pvdaq/ConfigMonitor.hh"
#include "pds/pvdaq/WaveformPvServer.hh"
#include "pds/config/AcqConfigType.hh"
#include "pds/config/AcqDataType.hh"
#include "pds/xtc/InDatagram.hh"
#include "pds/utility/Occurrence.hh"
#include "pds/service/GenericPool.hh"
#include <stdio.h>

static const unsigned EB_EVT_EXTRA = 0x10000;
static const unsigned NPRINT = 20;
static const unsigned PV_LEN = 64;
static const unsigned NCHAN  = 1;

#define CREATE_PV_NAME(pvname, fmt, ...)                \
  snprintf(pvname, PV_LEN, fmt, _pvchan, ##__VA_ARGS__);

#define CREATE_PV(pv, fmt, ...)                         \
  snprintf(pvname, PV_LEN, fmt, _pvchan, ##__VA_ARGS__); \
  printf("Creating EpicsCA(%s)\n",pvname);              \
  pv = new ConfigServer(pvname, _configMonitor);        \
  _config_pvs.push_back(pv);

#define CREATE_ENUM_PV(pv, fmt, ...)                    \
  snprintf(pvname, PV_LEN, fmt, _pvchan, ##__VA_ARGS__); \
  printf("Creating EpicsCA(%s)\n",pvname);              \
  pv = new ConfigServer(pvname, _configMonitor, true);  \
  _config_pvs.push_back(pv);

#define CREATE_CARD_PV(pv, fmt, ...)                    \
  snprintf(pvname, PV_LEN, fmt, _pvcard, ##__VA_ARGS__); \
  printf("Creating EpicsCA(%s)\n",pvname);              \
  pv = new ConfigServer(pvname, _configMonitor);        \
  _config_pvs.push_back(pv);

#define CREATE_CARD_ENUM_PV(pv, fmt, ...)               \
  snprintf(pvname, PV_LEN, fmt, _pvcard, ##__VA_ARGS__); \
  printf("Creating EpicsCA(%s)\n",pvname);              \
  pv = new ConfigServer(pvname, _configMonitor, true);  \
  _config_pvs.push_back(pv);

#define CHECK_PV(pv, val)                                         \
  if ((!pv->connected()) || (pv->fetch(&val, sizeof(val)) < 0)) { \
    printf("Error retrieving value of PV: %s\n", pv->name());     \
    error = true;                                                 \
  }

#define CHECK_STR_PV(pv, str, len)                            \
  if ((!pv->connected()) || (pv->fetch_str(str, len) < 0)) {  \
    printf("Error retrieving value of PV: %s\n", pv->name()); \
    str[0] = '\0';                                            \
    error = true;                                             \
  }

using namespace Pds::PvDaq;
using namespace Pds::Acqiris;
AcqirisServer::AcqirisServer(const char*          pvchan,
                             const char*          pvcard,
                             const Pds::DetInfo&  info,
                             const unsigned       max_event_size,
                             const unsigned       flags) :
  _pvchan     (new char[strlen(pvchan)+1]),
  _pvcard     (pvcard ? new char[strlen(pvcard)+1] : new char[strlen(pvchan)+1]),
  _info       (info),
  _cfg        (NULL),
  _waveformPv      (NULL),
  _fullScalePv     (NULL),
  _offsetPv        (NULL),
  _couplingPv      (NULL),
  _bandwidthPv     (NULL),
  _sampIntervalPv  (NULL),
  _delayTimePv     (NULL),
  _nbrSamplesPv    (NULL),
  _nbrSegmentsPv   (NULL),
  _trigCouplingPv  (NULL),
  _trigInputPv     (NULL),
  _trigSlopePv     (NULL),
  _trigLevelPv     (NULL),
  _nbrConvPerChanPv(NULL),
  _nbrBanksPv      (NULL),
  _data_pvname(new char[PV_LEN]),
  _fullScale       (0.0),
  _offset          (0.0),
  _coupling        (new char[PV_LEN]),
  _bandwidth       (new char[PV_LEN]),
  _sampInterval    (0.0),
  _delayTime       (0.0),
  _nbrSamples      (0),
  _nbrSegments     (0),
  _trigCoupling    (new char[PV_LEN]),
  _trigInput       (new char[PV_LEN]),
  _trigSlope       (new char[PV_LEN]),
  _trigLevel       (0.0),
  _nbrConvPerChan  (0),
  _nbrBanks        (0),
  _enabled    (false),
  _configured (false),
  _max_evt_sz (max_event_size),
  _waveform_sz(0),
  _configMonitor(new ConfigMonitor(*this)),
  _nprint     (NPRINT),
  _wrp        (0),
  _pool       (8),
  _context    (ca_current_context())
{
  _xtc = Xtc(_acqDataType, info);

  // intialize the pvbase names
  strcpy(_pvchan, pvchan);
  if (pvcard) {
    strcpy(_pvcard, pvcard);
  } else {
    // the normal scheme in the ioc is to use 0 as last digit for card pv
    strcpy(_pvcard, _pvchan);
    _pvcard[strlen(_pvcard)-1] = '0';
  }

  char pvname[PV_LEN];

  // the raw waveforms name
  CREATE_PV_NAME(_data_pvname, "%s:Data");
  // vert full scale pv
  CREATE_PV(_fullScalePv, "%s:CFullScale");
  // vert offset pv
  CREATE_PV(_offsetPv, "%s:COffset");
  // vert coupling pv
  CREATE_ENUM_PV(_couplingPv, "%s:CCoupling");
  // vert bandwidth pv
  CREATE_ENUM_PV(_bandwidthPv, "%s:CBandwidth");
  // horiz sampling interval pv
  CREATE_CARD_PV(_sampIntervalPv, "%s:MSampInterval");
  // horiz delay time pv
  CREATE_CARD_PV(_delayTimePv, "%s:MDelayTime");
  // horiz number of samples
  CREATE_CARD_PV(_nbrSamplesPv, "%s:MNbrSamples");
  // horiz number of segments
  CREATE_CARD_PV(_nbrSegmentsPv, "%s:MNbrSegments");
  // trig coupling pv
  CREATE_ENUM_PV(_trigCouplingPv, "%s:CTrigCoupling");
  // trig input pv
  CREATE_CARD_ENUM_PV(_trigInputPv, "%s:MTriggerSource");
  // trig slope pv
  CREATE_ENUM_PV(_trigSlopePv, "%s:CTrigSlope");
  // trig level pv
  CREATE_PV(_trigLevelPv, "%s:CTrigLevel1");
  // number of converters pv
  CREATE_CARD_PV(_nbrConvPerChanPv, "%s:MNbrConvertersPerChannel");
  // number of banks pv
  CREATE_CARD_PV(_nbrBanksPv, "%s:MNbrBanks");

  for(unsigned i=0; i<_pool.size(); i++) {
    _pool[i] = new char[_max_evt_sz];
    reinterpret_cast<Dgram*>(_pool[i])->seq = Sequence(ClockTime(0,0),
                                                       TimeStamp(0,0,0,0));
  }
  printf("Initialized Acqiris: %s\n", _pvchan);
}

AcqirisServer::~AcqirisServer()
{
  if (_cfg)
    delete _cfg;
  delete[] _pvchan;
  delete[] _pvcard;
  delete[] _data_pvname;
  delete[] _coupling;
  delete[] _bandwidth;
  delete[] _trigCoupling;
  delete[] _trigInput;
  delete[] _trigSlope;
  if (_fullScalePv)
    delete _fullScalePv;
  if (_offsetPv)
    delete _offsetPv;
  if (_couplingPv)
    delete _couplingPv;
  if (_bandwidthPv)
    delete _bandwidthPv;
  if (_sampIntervalPv)
    delete _sampIntervalPv;
  if (_delayTimePv)
    delete _delayTimePv;
  if (_nbrSamplesPv)
    delete _nbrSamplesPv;
  if (_nbrSegmentsPv)
    delete _nbrSegmentsPv;
  if (_trigCouplingPv)
    delete _trigCouplingPv;
  if (_trigInputPv)
    delete _trigInputPv;
  if (_trigSlopePv)
    delete _trigSlopePv;
  if (_trigLevelPv)
    delete _trigLevelPv;
  if (_nbrConvPerChanPv)
    delete _nbrConvPerChanPv;
  if (_nbrBanksPv)
    delete _nbrBanksPv;

  if (_waveformPv)
    delete _waveformPv;
  for(unsigned i=0; i<_pool.size(); i++) {
    if (_pool[i])
      delete[] _pool[i];
  }
}

int AcqirisServer::fill(char* payload, const void* p)
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

void AcqirisServer::updated()
{
  if (!_enabled)
    return;

  Dgram* dg = new (_pool[_wrp]) Dgram;

  //
  //  Parse the raw data and fill the copyTo payload
  //
  unsigned fiducial = _waveformPv->nsec()&0x1ffff;

  dg->seq = Pds::Sequence(Pds::Sequence::Event,
                          Pds::TransitionId::L1Accept,
                          Pds::ClockTime(_waveformPv->sec(),_waveformPv->nsec()),
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

    DataDescV1Elem* acqptr = new (dg->xtc.alloc(sizeof(DataDescV1Elem)))
      DataDescV1Elem(0);
    // add samplesperseg, first point, and number of segments to data
    uint32_t* sampptr = (uint32_t*) acqptr;
    *sampptr = _cfg->horiz().nbrSamples();
    uint32_t* fpptr = sampptr + 1;
    *fpptr = 0;
    uint32_t* segpptr = fpptr + 1 + (sizeof(double) * 3)/(sizeof(uint32_t));
    *segpptr = _cfg->horiz().nbrSegments();
    // fake the timestamp data using epics timestamp
    char* tsptr = ((char*) acqptr) + sizeof(DataDescV1Elem);
    new (tsptr) TimestampV1(0, _waveformPv->nsec(), _waveformPv->sec());
    // get a pointer to the start of the waveform data
    char* dataptr = tsptr + sizeof(TimestampV1);
    dg->xtc.alloc(acqptr->_sizeof(*_cfg) - sizeof(DataDescV1Elem));
    int fetch_sz = _waveformPv->fetch(dataptr, _waveform_sz);
    if (fetch_sz < 0) {
      if (_nprint) {
        printf("Error fetching waveform from %s\n", _waveformPv->name());
      }
      dg->xtc.damage.increase(Pds::Damage::UserDefined);
      dg->xtc.damage.userBits(2);
      break;
    } else if ((unsigned) fetch_sz != _waveform_sz) {
      if (_nprint) {
        printf("Frame data does not match expected size: read %d, expected %zu\n", fetch_sz, _waveform_sz);
        --_nprint;
      }
      dg->xtc.damage.increase(Pds::Damage::UserDefined);
      dg->xtc.damage.userBits(1);
      break;
    }
    len += acqptr->_sizeof(*_cfg);
  } while(0);

  dg->xtc.extent = len;

  post(dg);
}

Pds::Transition* AcqirisServer::fire(Pds::Transition* tr)
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

Pds::InDatagram* AcqirisServer::fire(Pds::InDatagram* dg)
{
  if (dg->seq.service()==Pds::TransitionId::Configure) {
    _last = ClockTime(0,0);
    _wrp  = 0;

    for(unsigned i=0; i<_pool.size(); i++)
      reinterpret_cast<Dgram*>(_pool[i])->seq = Sequence(ClockTime(0,0),
                                                         TimeStamp(0,0,0,0));

    bool error = false;
    VertV1::Coupling coupling_ddl = VertV1::GND;
    VertV1::Bandwidth bandwidth_ddl = VertV1::None;
    TrigV1::Source trigInput_ddl = TrigV1::Internal;
    TrigV1::Coupling trigCoupling_ddl = TrigV1::DC;
    TrigV1::Slope trigSlope_ddl = TrigV1::Positive;
    printf("Retrieving acqiris configuration information from epics...\n");
    // check VertV1 pvs
    CHECK_PV    (_fullScalePv,    _fullScale);
    CHECK_PV    (_offsetPv,       _offset);
    CHECK_STR_PV(_couplingPv,     _coupling, PV_LEN);
    if (!strcmp(_coupling, "Ground")) {
      coupling_ddl = VertV1::GND;
    } else if (!strcmp(_coupling, "DC")) {
      coupling_ddl = VertV1::DC;
    } else if (!strcmp(_coupling, "AC")) {
      coupling_ddl = VertV1::AC;
    } else if (!strcmp(_coupling, "DC_50_Ohm")) {
     coupling_ddl = VertV1::DC50ohm;
    } else if (!strcmp(_coupling, "AC_50_Ohm")) { 
      coupling_ddl = VertV1::AC50ohm;
    } else {
      printf("Error: failed to parse VertV1 coupling enum value: %s\n",
             _coupling);
      error = true;
    }
    CHECK_STR_PV(_bandwidthPv,    _bandwidth, PV_LEN);
    if (!strcmp(_bandwidth, "No_Limit")) {
      bandwidth_ddl = VertV1::None;
    } else if (!strcmp(_bandwidth, "25Mhz")) {
      bandwidth_ddl = VertV1::MHz25;
    } else if (!strcmp(_bandwidth, "700MHz")) {
      bandwidth_ddl = VertV1::MHz700;
    } else if (!strcmp(_bandwidth, "200Mhz")) {
      bandwidth_ddl = VertV1::MHz200;
    } else if (!strcmp(_bandwidth, "20Mhz")) {
      bandwidth_ddl = VertV1::MHz20;
    } else if (!strcmp(_bandwidth, "35Mhz")) {
      bandwidth_ddl = VertV1::MHz35;
    } else {
      printf("Error: failed to parse VertV1 bandwidth enum value: %s\n",
             _bandwidth);
      error = true;
    }
    // check HorizV1 pvs
    CHECK_PV    (_sampIntervalPv, _sampInterval);
    CHECK_PV    (_delayTimePv,    _delayTime);
    CHECK_PV    (_nbrSamplesPv,   _nbrSamples);
    CHECK_PV    (_nbrSegmentsPv,  _nbrSegments);
    // check TrigV1 pvs
    CHECK_STR_PV(_trigCouplingPv, _trigCoupling, PV_LEN);
    if (!strcmp(_trigCoupling, "DC")) {
      trigCoupling_ddl = TrigV1::DC;
    } else if (!strcmp(_trigCoupling, "AC")) {
      trigCoupling_ddl = TrigV1::AC;
    } else if (!strcmp(_trigCoupling, "HF_Reject")) {
      trigCoupling_ddl = TrigV1::HFreject;
    } else if (!strcmp(_trigCoupling, "DC_50_Ohm")) {
      trigCoupling_ddl = TrigV1::DC50ohm;
    } else if (!strcmp(_trigCoupling, "AC_50_Ohm")) {
      trigCoupling_ddl = TrigV1::AC50ohm;
    } else {
      printf("Error: failed to parse TrigV1 coupling enum value: %s\n",
             _trigCoupling);
      error = true;
    }
    CHECK_STR_PV(_trigInputPv,    _trigInput, PV_LEN);
    if (!strcmp(_trigInput, "External")) {
      trigInput_ddl = TrigV1::External;
    } else if (!strcmp(_trigInput, "Channel1") ||
               !strcmp(_trigInput, "Channel2") ||
               !strcmp(_trigInput, "Channel3") ||
               !strcmp(_trigInput, "Channel4")) {
      trigInput_ddl = TrigV1::Internal;
    } else {
      printf("Error: failed to parse TrigV1 input enum value: %s\n",
             _trigInput);
      error = true;
    }
    CHECK_STR_PV(_trigSlopePv,    _trigSlope, PV_LEN);
    if (!strcmp(_trigSlope, "Positive")) {
      trigSlope_ddl = TrigV1::Positive;
    } else if (!strcmp(_trigSlope, "Negative")) {
      trigSlope_ddl = TrigV1::Negative;
    } else if (!strcmp(_trigSlope, "Out_of_Window")) {
      trigSlope_ddl = TrigV1::OutOfWindow;
    } else if (!strcmp(_trigSlope, "Into_Window")) {
      trigSlope_ddl = TrigV1::IntoWindow;
    } else if (!strcmp(_trigSlope, "HF_Divide")) {
      trigSlope_ddl = TrigV1::HFDivide;
    } else if (!strcmp(_trigSlope, "Spike_Stretcher")) {
      trigSlope_ddl = TrigV1::SpikeStretcher;
    } else {
      printf("Error: failed to parse TrigV1 slope enum value: %s\n",
             _trigSlope);
      error = true;
    }
    CHECK_PV    (_trigLevelPv,    _trigLevel);
    // check device level pvs
    CHECK_PV    (_nbrConvPerChanPv, _nbrConvPerChan);
    CHECK_PV    (_nbrBanksPv,       _nbrBanks);

    printf("Acqiris configuration information:\n"
           "  VertV1:\n"
           "    fullScale       %g\n"
           "    offset          %g\n"
           "    coupling        %s\n"
           "    bandwidth       %s\n"
           "  HorizV1:\n"
           "    sampInterval    %g\n"
           "    delayTime       %g\n"
           "    nbrSamples      %u\n"
           "    nbrSegments     %u\n"
           "  TrigV1:\n"
           "    trigCoupling    %s\n"
           "    trigInput       %s\n"
           "    trigSlope       %s\n"
           "    trigLevel       %g\n"
           "  nbrConvPerChan    %u\n"
           "  nbrBanks          %u\n",
           _fullScale,
           _offset,
           _coupling,
           _bandwidth,
           _sampInterval,
           _delayTime,
           _nbrSamples,
           _nbrSegments,
           _trigCoupling,
           _trigInput,
           _trigSlope,
           _trigLevel,
           _nbrConvPerChan,
           _nbrBanks);

    if (!_waveformPv) {
      if (ca_current_context() == NULL) ca_attach_context(_context);
      printf("Creating EpicsCA(%s)\n", _data_pvname);
      _waveformPv = new AcqirisPvServer(_data_pvname, this, _nbrSamples);
    }

    // create TrigV1 cfg
    TrigV1 trigCfg(trigCoupling_ddl,
                   trigInput_ddl,
                   trigSlope_ddl,
                   _trigLevel);
    
    // create HorizV1 cfg
    HorizV1 horizCfg(_sampInterval,
                     _delayTime,
                     _nbrSamples,
                     _nbrSegments);
    
    VertV1 vertCfg(_fullScale,
                   _offset,
                   coupling_ddl,
                   bandwidth_ddl);
    
    Pds::Xtc* xtc;
    xtc = new ((char*)dg->xtc.next())
      Pds::Xtc(_acqConfigType, _xtc.src);
    AcqConfigType* cfg = new (xtc->alloc(sizeof(AcqConfigType)))
      AcqConfigType(_nbrConvPerChan,
                    NCHAN,
                    _nbrBanks,
                    trigCfg,
                    horizCfg,
                    &vertCfg);
    dg->xtc.alloc(xtc->extent);

    // cache the acq configuration
    if (_cfg) delete _cfg;
    _cfg = new AcqConfigType(*cfg);

    _waveform_sz = _nbrSamples * sizeof(uint16_t);

    if (!error) {
      if (_waveform_sz > _max_evt_sz) {
        printf("Error: EPICS_CA_MAX_ARRAY_BYTES not large enough for expected waveform size %zu vs %u\n", _waveform_sz, _max_evt_sz);
        error = true;
      } else if ((_waveform_sz + EB_EVT_EXTRA) > _max_evt_sz) {
        printf("Error: expected event size will overrun eventbuilder buffer %zu vs. %u\n", (_waveform_sz + EB_EVT_EXTRA), _max_evt_sz);
      }
    }

    if (error) {
      printf("Acqiris configuration failed!\n");
      char msg[128];
      snprintf(msg, 128, "Acqiris[%s] config error!", _pvchan);
      Pds::UserMessage* umsg = new (_occPool) Pds::UserMessage(msg);
      sendOcc(umsg);
      dg->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
    }
  } else if (dg->seq.service()==Pds::TransitionId::Unconfigure) {
    if (_waveformPv) {
      if (ca_current_context() == NULL) ca_attach_context(_context);
      delete _waveformPv;
      _waveformPv = 0;
    }
  }

  return dg;
}

void AcqirisServer::config_updated()
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
#undef CREATE_CARD_PV
#undef CREATE_CARD_ENUM_PV
#undef CHECK_PV
#undef CHECK_STR_PV
