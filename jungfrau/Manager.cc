#include "pds/jungfrau/Manager.hh"
#include "pds/jungfrau/Driver.hh"
#include "pds/jungfrau/Server.hh"
#include "pds/jungfrau/DetectorId.hh"
#include "pds/jungfrau/ConfigCache.hh"

#include "pds/config/JungfrauConfigType.hh"
#include "pds/config/JungfrauDataType.hh"
#include "pds/client/Action.hh"
#include "pds/client/Fsm.hh"
#include "pds/utility/Appliance.hh"
#include "pds/service/GenericPool.hh"
#include "pds/service/Task.hh"
#include "pds/service/RingPool.hh"
#include "pdsdata/xtc/XtcIterator.hh"

#include <vector>
#include <errno.h>
#include <math.h>

namespace Pds {
  namespace Jungfrau {
    class FrameReader : public Routine {
    public:
      FrameReader(Detector& detector, Server& server, Task* task) :
        _task(task),
        _detector(detector),
        _server(server),
        _disable(true),
        _abort(false),
        _header_sz(0),
        _frame_sz(0),
        _entry_sz(0),
        _buffer(0),
        _frame_ptr(0),
        _framenum_ptr(0),
        _metadata_ptr(0)
      {
        _header_sz = sizeof(JungfrauModuleInfoType) * _detector.get_num_modules();
        _frame_sz = _detector.get_frame_size();
        _entry_sz = sizeof(JungfrauDataType) + _header_sz + _frame_sz;
        _buffer = new char[_entry_sz];
        _server.set_frame_sz(_header_sz + _frame_sz);
      }
      virtual ~FrameReader() {
        if (_buffer) delete[] _buffer;
      }
      Task& task() { return *_task; }
      void enable () { _disable=false; _task->call(this); }
      void disable() { _disable=true ; }
      void abort() { _abort=true; _detector.abort(); }
      void routine() {
        if (_disable) {
          _abort=false;
        } else {
          _frame_ptr = (uint16_t*) (_buffer + sizeof(JungfrauDataType) + _header_sz);
          _framenum_ptr = (uint64_t*) _buffer;
          _metadata_ptr = (JungfrauModInfoType*) (_buffer + sizeof(JungfrauDataType));
          if (_detector.get_frame(_framenum_ptr, _metadata_ptr, _frame_ptr)) {
            _server.post(_buffer);
          } else if(_detector.aborted() && !_abort) { // external abort so disable reader
            _disable = true;
          } else if(!_abort) {
            fprintf(stderr, "Error: FrameReader failed to retrieve frame from Jungfrau receiver\n");
          }
          _task->call(this);
        }
      }
    private:
      Task*     _task;
      Detector& _detector;
      Server&   _server;
      bool      _disable;
      bool      _abort;
      unsigned  _header_sz;
      unsigned  _frame_sz;
      unsigned  _entry_sz;
      char*     _buffer;
      uint16_t* _frame_ptr;
      uint64_t* _framenum_ptr;
      JungfrauModInfoType* _metadata_ptr;
    };

    class ReaderEnable : public Routine {
    public:
      ReaderEnable(FrameReader& reader): _reader(reader), _sem(Semaphore::EMPTY) { }
      ~ReaderEnable() { }
      void call   () { _reader.task().call(this); _sem.take(); }
      void routine() { _reader.enable(); _sem.give(); }
    private:
      FrameReader&  _reader;
      Semaphore     _sem;
    };

    class ReaderDisable : public Routine {
    public:
      ReaderDisable(FrameReader& reader): _reader(reader), _sem(Semaphore::EMPTY) { }
      ~ReaderDisable() { }
      void call   () { _reader.task().call(this);  _reader.abort(); _sem.take(); }
      void routine() { _reader.disable(); _sem.give(); }
    private:
      FrameReader&  _reader;
      Semaphore     _sem;
    };

    class AllocAction : public Action {
    public:
      AllocAction(ConfigCache& cfg) : _cfg(cfg) {}
      Transition* fire(Transition* tr) {
        const Allocate& alloc = reinterpret_cast<const Allocate&>(*tr);
        _cfg.init(alloc.allocation());
        return tr;
      }
    private:
      ConfigCache& _cfg;
    };

    class L1Action : public Action, public Pds::XtcIterator {
    public:
      static const unsigned skew_min_time = 0x2fffffff;
      static const unsigned fid_rollover_secs = Pds::TimeStamp::MaxFiducials / 360;
      L1Action(unsigned num_modules, Manager& mgr) :
        _mgr(mgr),
        _lreset(true),
        _synced(true),
        _sync_msg(true),
        _unsycned_count(0),
        _num_modules(num_modules),
        _dgm_ts(0),
        _nfid(0),
        _mod_ts(new uint64_t[num_modules]),
        _skew(new double[num_modules]),
        _occPool(sizeof(UserMessage),4) {}
      ~L1Action() {
        if (_mod_ts) delete[] _mod_ts;
        if (_skew) delete[] _skew;
      }
      InDatagram* fire(InDatagram* in) {
        unsigned dgm_ts = in->datagram().seq.stamp().fiducials();
        ClockTime dgm_time = in->datagram().seq.clock();
        unsigned nrollover = unsigned((dgm_time.asDouble() - _dgm_time.asDouble()) / fid_rollover_secs);
        if (nrollover > 0) {
          _nfid = (Pds::TimeStamp::MaxFiducials * nrollover + dgm_ts) - _dgm_ts;
        } else {
          _nfid = dgm_ts - _dgm_ts;
          if (((signed) _nfid) < 0)
            _nfid += Pds::TimeStamp::MaxFiducials;
        }

        _in = in;
        iterate(&in->datagram().xtc);

        return _in;
      }
      void sync() { _synced=true; _unsycned_count=0; _sync_msg=true; }
      void reset() { _lreset=true; }
      uint64_t diff_ts(uint64_t t1, uint64_t t2) {
        if (t1 > t2)
          return t1-t2;
        else
          return t2-t1;
      }
      int process(Xtc* xtc) {
        if (xtc->contains.id()==TypeId::Id_Xtc)
          iterate(xtc);
        else if (xtc->contains.value() == _jungfrauDataType.value()) {
          uint32_t* ptr = (uint32_t*) (xtc->payload() + sizeof(uint64_t));
          JungfrauModInfoType* mod_info = (JungfrauModInfoType*) (xtc->payload() + sizeof(JungfrauDataType));
          ptr[0] = _in->datagram().seq.stamp().ticks();
          ptr[1] = _in->datagram().seq.stamp().fiducials();

          if (_lreset) {
            _lreset = false;
            _dgm_ts   = _in->datagram().seq.stamp().fiducials();
            _dgm_time = _in->datagram().seq.clock();
            // reset module timestamps and skews
            for (unsigned i=0; i<_num_modules; i++) {
              _mod_ts[i] = mod_info[i].timestamp();
              _skew[i] = 1.0;
            }
          } else {
            bool mods_synced = true;
            bool frames_synced = true;
            uint64_t max_ts = 0;
            const double clkratio  = 360./10e6;
            const double tdiff = 2.5e-5;    // the max allowed time difference (in seconds) between different modules in a frame
            const double tolerance = 0.003; // AC line rate jitter and Jungfrau clock drift
            const unsigned maxdfid = 21600; // if there is more than 1 minute between triggers
            const unsigned maxunsync = 240;

            for (unsigned i=0; i<_num_modules; i++) {
              double fdelta = double(mod_info[i].timestamp() - _mod_ts[i])*clkratio/double(_nfid) - 1;
              if (fabs(fdelta) > tolerance && (_nfid < maxdfid || !_synced)) {
                unsigned nfid = unsigned(double(mod_info[i].timestamp() - _mod_ts[i])*clkratio + 0.5);
                printf("  timestep error for module %u: fdelta %f  dfid %d  tds %lu,%lu [%d]\n", i, fdelta, _nfid, mod_info[i].timestamp(), _mod_ts[i], nfid);
                mods_synced = false;
              } else if (mod_info[i].timestamp() > max_ts) {
                max_ts = mod_info[i].timestamp();
              }
            }

            if (mods_synced) {
              // check for large timing differences between the modules
              for (unsigned i=0; i<_num_modules; i++) {
                double fdelta = (max_ts - (_skew[i] * mod_info[i].timestamp())) / 10e6;
                if (fabs(fdelta) > tdiff && max_ts > skew_min_time && (_nfid < maxdfid || !_synced)) {
                  printf("  framesync error for module %u: offset of %g seconds\n", i, fabs(fdelta));
                  frames_synced = false;
                } else {
                  _skew[i] = double(max_ts) / mod_info[i].timestamp();
                }
              }
            }

            // set the synced status based on results from looping over the modules
            _synced = mods_synced && frames_synced;

            if (!_synced) {
              xtc->damage.increase(Pds::Damage::OutOfSynch);
              // add the damage to the L1Accept datagram as well
              _in->datagram().xtc.damage.increase(xtc->damage.value());
              _unsycned_count++;
              if ((_unsycned_count > maxunsync) && _sync_msg) {
                printf("L1Action: Detector has been out of sync for %u frames - reconfiguration needed!\n", _unsycned_count);
                UserMessage* msg = new (&_occPool) UserMessage("Jungfrau Error: Detector is out of sync - reconfiguration needed!\n");
                _mgr.appliance().post(msg);
                Occurrence* occ = new(&_occPool) Occurrence(OccurrenceId::ClearReadout);
                _mgr.appliance().post(occ);
                _sync_msg = false;
              }
            } else {
              _unsycned_count = 0;
              _dgm_ts   = _in->datagram().seq.stamp().fiducials();
              _dgm_time = _in->datagram().seq.clock();
              for (unsigned i=0; i<_num_modules; i++) {
                _mod_ts[i] = mod_info[i].timestamp();
              }
            }
          }
          return 0;
        }
        return 1;
      }
    private:
      Manager&    _mgr;
      bool        _lreset;
      bool        _synced;
      bool        _sync_msg;
      unsigned    _unsycned_count;
      unsigned    _num_modules;
      unsigned    _dgm_ts;
      unsigned    _nfid;
      uint64_t*   _mod_ts;
      double*     _skew;
      ClockTime   _dgm_time;
      GenericPool _occPool;
      InDatagram* _in;
    };

    class ConfigAction : public Action {
    public:
      ConfigAction(Manager& mgr, Detector& detector, Server& server, FrameReader& reader,
                   ConfigCache& cfg, L1Action& l1) :
        _mgr(mgr),
        _detector(detector),
        _server(server),
        _cfg(cfg),
        _l1(l1),
        _enable(reader),
        _occPool(sizeof(UserMessage),1) {}
      ~ConfigAction() {}
      InDatagram* fire(InDatagram* dg) {
        _cfg.record(dg);
        return dg;
      }
      Transition* fire(Transition* tr) {

        int len = _cfg.fetch(tr);
        if (len <= 0) {
          printf("ConfigAction: failed to retrieve configuration: (%d) %s.\n", errno, strerror(errno));

          UserMessage* msg = new (&_occPool) UserMessage("Jungfrau Config Error: failed to retrieve configuration.\n");
          _mgr.appliance().post(msg);
        } else {
          _server.resetCount();
          if (_cfg.scanning()) {
            // update the config object in the transition but don't apply to detector
            _cfg.configure(false);
          } else {
            if (_cfg.configure()) {
              _server.setFrame(_detector.sync_nframes());
              _l1.reset();
              _detector.flush();
              _enable.call();
            }
          }
        }

        return tr;
      }
    private:
      Manager&              _mgr;
      Detector&             _detector;
      Server&               _server;
      ConfigCache&          _cfg;
      L1Action&             _l1;
      ReaderEnable          _enable;
      GenericPool           _occPool;
    };

    class BeginCalibCycleAction : public Action {
    public:
      BeginCalibCycleAction(Detector& detector, Server& server, FrameReader& reader,
                            ConfigCache& cfg, L1Action& l1):
        _detector(detector),
        _server(server),
        _cfg(cfg),
        _l1(l1),
        _enable(reader) {}
      ~BeginCalibCycleAction() {}
      InDatagram* fire(InDatagram* dg) {
        if (_cfg.scanning() && _cfg.changed()) {
          _cfg.record(dg);
        }
        return dg;
      }
      Transition* fire(Transition* tr) {
        if (_cfg.scanning()) {
          bool error = false;
          if (_cfg.changed()) {
            error = !_cfg.configure();
          }

          if (error) {
            printf("BeginCalibCycleAction: failed to configure Jungfrau during scan!\n");
          } else {
            _server.setFrame(_detector.sync_nframes());
            _l1.reset();
            _detector.flush();
            _enable.call();
          }
        }

        return tr;
      }
    private:
        Detector&             _detector;
        Server&               _server;
        ConfigCache&          _cfg;
        L1Action&             _l1;
        ReaderEnable          _enable;
    };

    class EndCalibCycleAction : public Action {
    public:
      EndCalibCycleAction(FrameReader& reader, ConfigCache& cfg):
        _disable(reader),
        _cfg(cfg) {}
      ~EndCalibCycleAction() {}
      InDatagram* fire(InDatagram* dg) {
        return dg;
      }
      Transition* fire(Transition* tr) {
        _cfg.next();
        if (_cfg.scanning()) {
          _disable.call();
        }
        return tr;
      }
    private:
      ReaderDisable _disable;
      ConfigCache&  _cfg;
    };

    class UnconfigureAction : public Action {
    public:
      UnconfigureAction(FrameReader& reader, ConfigCache& cfg):
        _disable(reader),
        _cfg(cfg) {}
      ~UnconfigureAction() {}
      InDatagram* fire(InDatagram* dg) {
        return dg;
      }
      Transition* fire(Transition* tr) {
        if (!_cfg.scanning()) {
          _disable.call();
        }
        return tr;
      }
    private:
      ReaderDisable _disable;
      ConfigCache&  _cfg;
    };

    class EnableAction : public Action {
    public:
      EnableAction(Detector& detector, L1Action& l1): 
        _detector(detector),
        _l1(l1),
        _error(false) { }
      ~EnableAction() { }
      InDatagram* fire(InDatagram* dg) {
        if (_error) {
          printf("EnableAction: failed to enable Jungfrau.\n");
          dg->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
        }
        return dg;
      }
      Transition* fire(Transition* tr) {
        _l1.sync();
        _error = !_detector.start();  
        return tr;
      }
    private:
      Detector&     _detector;
      L1Action&     _l1;
      bool          _error;
    };

    class DisableAction : public Action {
    public:
      DisableAction(Detector& detector): _detector(detector), _error(false) { }
      ~DisableAction() { }
      InDatagram* fire(InDatagram* dg) {
        if (_error) {
          printf("DisableAction: failed to disable Jungfrau.\n");
          dg->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
        }
        return dg;
      }
      Transition* fire(Transition* tr) {
        _error = !_detector.stop();
        return tr;
      }
    private:
      Detector&     _detector;
      bool          _error;
    };
  }
}

using namespace Pds::Jungfrau;

Manager::Manager(const Src& configSrc, Detector& detector, Server& server, DetIdLookup& lookup) :
  _fsm(*new Pds::Fsm())
{
  Task* task = new Task(TaskObject("JungfrauReadout",35));
  L1Action* l1 = new L1Action(detector.get_num_modules(), *this);
  FrameReader& reader = *new FrameReader(detector, server,task);
  ConfigCache& cfg = *new ConfigCache(configSrc, server.client(), detector, lookup, *this);

  _fsm.callback(Pds::TransitionId::Map,             new AllocAction(cfg));
  _fsm.callback(Pds::TransitionId::Configure,       new ConfigAction(*this, detector, server, reader, cfg, *l1));
  _fsm.callback(Pds::TransitionId::Enable,          new EnableAction(detector, *l1));
  _fsm.callback(Pds::TransitionId::Disable,         new DisableAction(detector));
  _fsm.callback(Pds::TransitionId::BeginCalibCycle, new BeginCalibCycleAction(detector, server, reader, cfg, *l1));
  _fsm.callback(Pds::TransitionId::EndCalibCycle,   new EndCalibCycleAction(reader, cfg));
  _fsm.callback(Pds::TransitionId::Unconfigure,     new UnconfigureAction(reader, cfg));
  _fsm.callback(Pds::TransitionId::L1Accept,        l1);
}

Manager::~Manager() {}

Pds::Appliance& Manager::appliance() {return _fsm;}

