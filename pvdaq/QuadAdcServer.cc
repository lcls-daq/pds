#include "pds/pvdaq/QuadAdcServer.hh"
#include "pds/pvdaq/ConfigMonitor.hh"
#include "pds/pvdaq/WaveformPvServer.hh"
#include "pds/config/Generic1DConfigType.hh"
#include "pds/config/Generic1DDataType.hh"
#include "pds/config/QuadAdcConfigType.hh"
#include "pds/xtc/InDatagram.hh"
#include "pds/utility/Occurrence.hh"
#include "pds/service/GenericPool.hh"
#include <stdio.h>

static const unsigned EB_EVT_EXTRA = 0x10000;
static const unsigned NPRINT = 20;
static const unsigned PV_LEN = 64;
static const unsigned NCHAN  = 4;

#define CREATE_PV_NAME(pvname, fmt, ...)                \
  snprintf(pvname, PV_LEN, fmt, pvbase, ##__VA_ARGS__);

#define CREATE_PV(pv, fmt, ...)                         \
  snprintf(pvname, PV_LEN, fmt, pvbase, ##__VA_ARGS__); \
  printf("Creating EpicsCA(%s)\n",pvname);              \
  pv = new ConfigServer(pvname, _configMonitor);        \
  _config_pvs.push_back(pv);

#define CHECK_PV(pv, val)                                         \
  if ((!pv->connected()) || (pv->fetch(&val, sizeof(val)) < 0)) { \
    printf("Error retrieving value of PV: %s\n", pv->name());     \
    error = true;                                                 \
  }

using namespace Pds::PvDaq;

QuadAdcServer::QuadAdcServer(const char*          pvbase,
                             const char*          pvdata,
                             const Pds::DetInfo&  info,
                             const unsigned       max_event_size,
                             const unsigned       flags) :
  _pvbase     (pvbase),
  _pvdata     (pvdata ? pvdata : "RAWDATA"),
  _info       (info),
  _waveformPv (NULL),
  _ichanPv    (NULL),
  _trigEventPv(NULL),
  _trigDelayPv(NULL),
  _lengthPv   (NULL),
  _interleavePv(NULL),
  _data_pvname(new char[PV_LEN]),
  _inputChan  (0),
  _chanMask   (0),
  _evtCode    (0),
  _evtDelay   (0),
  _delayTime  (0.0),
  _nbrSamples (0),
  _interleave (0),
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
  _xtc = Xtc(_generic1DDataType, info);

  char pvname[PV_LEN];

  // the raw waveforms name
  CREATE_PV_NAME(_data_pvname, "%s:%s", _pvdata);
  // input channel config pv
  CREATE_PV(_ichanPv, "%s:ICHAN");
  // trigger event config pv
  CREATE_PV(_trigEventPv, "%s:TRIG_EVENT");
  // trigger delay config pv
  CREATE_PV(_trigDelayPv, "%s:TRIG_DELAY");
  // number of samples config pv
  CREATE_PV(_lengthPv, "%s:LENGTH");
  // interleave mode config pv
  CREATE_PV(_interleavePv, "%s:INTERLEAVE");

  for(unsigned i=0; i<_pool.size(); i++) {
    _pool[i] = new char[_max_evt_sz];
    reinterpret_cast<Dgram*>(_pool[i])->seq = Sequence(ClockTime(0,0),
                                                       TimeStamp(0,0,0,0));
  }
  printf("Initialized QuadAdc: %s\n", _pvbase);
}

QuadAdcServer::~QuadAdcServer()
{
  delete[] _data_pvname;
  if (_ichanPv)
    delete _ichanPv;
  if (_trigEventPv)
    delete _trigEventPv;
  if (_trigDelayPv)
    delete _trigDelayPv;
  if (_lengthPv)
    delete _lengthPv;
  if (_interleavePv)
    delete _interleavePv;
  if (_waveformPv)
    delete _waveformPv;
  for(unsigned i=0; i<_pool.size(); i++) {
    if (_pool[i])
      delete[] _pool[i];
  }
}

int QuadAdcServer::fill(char* payload, const void* p)
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

void QuadAdcServer::updated()
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

    Generic1DDataType* waveptr = new (dg->xtc.alloc(sizeof(Generic1DDataType)))
      Generic1DDataType(_waveform_sz, NULL);
    char* dataptr = ((char*) waveptr) + sizeof(Generic1DDataType);
    dg->xtc.alloc(waveptr->_sizeof()-sizeof(Generic1DDataType));
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
    len += waveptr->_sizeof();
  } while(0);

  dg->xtc.extent = len;

  post(dg);
}

Pds::Transition* QuadAdcServer::fire(Pds::Transition* tr)
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

Pds::InDatagram* QuadAdcServer::fire(Pds::InDatagram* dg)
{
  if (dg->seq.service()==Pds::TransitionId::Configure) {
    _last = ClockTime(0,0);
    _wrp  = 0;

    for(unsigned i=0; i<_pool.size(); i++)
      reinterpret_cast<Dgram*>(_pool[i])->seq = Sequence(ClockTime(0,0),
                                                         TimeStamp(0,0,0,0));

    bool error = false;
    double period = 0.0;
    unsigned nchan = 0;
    uint32_t lengths[NCHAN];
    uint32_t types[NCHAN];
    int32_t offsets[NCHAN];
    double periods[NCHAN];
    printf("Retrieving quadadc configuration information from epics...\n");
    CHECK_PV(_ichanPv,      _inputChan);
    CHECK_PV(_trigEventPv,  _evtCode);
    CHECK_PV(_trigDelayPv,  _evtDelay);
    CHECK_PV(_lengthPv,     _nbrSamples);
    CHECK_PV(_interleavePv, _interleave);

    if(_interleave) {
      nchan = 1;
      _chanMask = 1<<_inputChan;
      _sampleRate = 5.0e-9;
    } else {
      nchan = NCHAN;
      _chanMask = (1<<NCHAN) - 1;
      _sampleRate = 1.25e-9;
    }
    _delayTime = _evtDelay / (156.17e6);
    period = 1.0/_sampleRate;
    for(unsigned n=0;n<nchan;n++) {
      lengths[n] = _nbrSamples;
      types[n] = Generic1DConfigType::FLOAT64;
      offsets[n] = unsigned(_delayTime / period + 0.5);
      periods[n] = period;
    }

    printf("Quad configuration information:\n" 
           "  channel mask      0x%x\n"
           "  input channel     %u\n"
           "  trig evt code     %u\n"
           "  trig delay (s)    %g\n"
           "  number of samples %u\n"
           "  sample period     %g\n"
           "  interleaved       %u\n",
           _chanMask,
           _inputChan,
           _evtCode,
           _delayTime,
           _nbrSamples,
           period,
           _interleave);

    if (!_waveformPv) {
      if (ca_current_context() == NULL) ca_attach_context(_context);
      printf("Creating EpicsCA(%s)\n", _data_pvname);
      _waveformPv = new QuadAdcPvServer(_data_pvname, this, nchan, _nbrSamples);
    }

    Pds::Xtc* xtc;

    xtc = new ((char*)dg->xtc.next())
      Pds::Xtc(_generic1DConfigType, _xtc.src);
    Generic1DConfigType* cfg = new (xtc->alloc(sizeof(Generic1DConfigType)))
      Generic1DConfigType(nchan, lengths, types, offsets, periods);
    xtc->alloc(cfg->_sizeof()-sizeof(Generic1DConfigType));
    dg->xtc.alloc(xtc->extent);

    _waveform_sz = nchan * _nbrSamples * sizeof(double);

    if (!error) {
      if (_waveform_sz > _max_evt_sz) {
        printf("Error: EPICS_CA_MAX_ARRAY_BYTES not large enough for expected waveform size %zu vs %u\n", _waveform_sz, _max_evt_sz);
        error = true;
      } else if ((_waveform_sz + EB_EVT_EXTRA) > _max_evt_sz) {
        printf("Error: expected event size will overrun eventbuilder buffer %zu vs. %u\n", (_waveform_sz + EB_EVT_EXTRA), _max_evt_sz);
      }
    }

    if (error) {
      printf("Quad configuration failed!\n");
      char msg[128];
      snprintf(msg, 128, "QuadAdc[%s] config error!", _pvbase);
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

void QuadAdcServer::config_updated()
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
#undef CHECK_PV
