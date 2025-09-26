#include "pds/zyla/Manager.hh"
#include "pds/zyla/Driver.hh"
#include "pds/zyla/Server.hh"
#include "pds/zyla/ConfigCache.hh"

#include "pds/config/ZylaConfigType.hh"
#include "pds/config/ZylaDataType.hh"
#include "pds/config/CfgClientNfs.hh"
#include "pds/client/Action.hh"
#include "pds/client/Fsm.hh"
#include "pds/utility/Appliance.hh"
#include "pds/service/GenericPool.hh"
#include "pds/service/Task.hh"
#include "pdsdata/xtc/XtcIterator.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include <errno.h>
#include <math.h>


#define AT_MAX_MSG_LEN 256
#define TIME_DIFF(start, end) (end.tv_nsec - start.tv_nsec) * 1.e-6 + (end.tv_sec - start.tv_sec) * 1.e3
//#define TIMING_DEBUG 1


namespace Pds {
  namespace Zyla {
    class FrameReader : public Routine {
    public:
      FrameReader(Driver& driver, Server& server, Task* task) :
        _task(task),
        _driver(driver),
        _server(server),
        _disable(true),
        _running(false),
        _current_frame(0),
        _last_frame(0),
        _diff_frame(0),
        _clock_rate(0),
        _num_diff(0),
        _frame_sz(0),
        _buffer(0),
        _frame_ptr(0),
        _framenum_ptr(0) {}
      virtual ~FrameReader() {
        if (_buffer) delete[] _buffer;
      }
      void enable () {
        _disable=false;
        _last_frame=0;
        if(!_running) {
          _running = true;
          _task->call(this);
        }
      }
      void disable() { _disable=true ; }
      void reset_diff() { _diff_frame=0; _num_diff=0; }
      void set_clock_rate(AT_64 clock_rate) { _clock_rate=clock_rate; }
      AT_64 convert_time(AT_64 clock_ticks) {
        if (_clock_rate < 1) {
          fprintf(stderr, "Error: FrameReader invalid clock rate for conversion: %lld\n", _clock_rate);
          return clock_ticks;
        } else {
          return (1000. * clock_ticks) / _clock_rate;
        }
      }
      void set_frame_sz(size_t sz) {
        if (_buffer) {
          if (sz > _frame_sz) {
            delete[] _buffer;
            _buffer = new char[sizeof(ZylaDataType) + sz];
          }
        } else {
          _buffer = new char[sizeof(ZylaDataType) + sz];
        }
        _frame_sz = sz;
      }
      void routine() {
        if (_disable) {
          _running = false;
        } else {
          _frame_ptr = (uint16_t*) (_buffer + sizeof(ZylaDataType));
          _framenum_ptr = (uint64_t*) _buffer;
          if (_driver.get_frame(_current_frame, _frame_ptr)) {
            *_framenum_ptr = (uint64_t) _current_frame;
            _server.post(_buffer);
            if (_last_frame != 0) {
              if (_diff_frame != 0) {
                if ((_current_frame - _last_frame)  > ((3 * _diff_frame) / 2)) {
                  if ((_num_diff < 10) || (_num_diff % 100 == 0)) {
                    fprintf(stderr, "Warning: FrameReader exceptional period: got period of %lld ms, but expected %lld ms\n",
                            convert_time(_current_frame - _last_frame), convert_time(_diff_frame));
                  }
                  _num_diff++;
                } else {
                  _num_diff = 0; // saw expected time diff reset count
                }
              } else {
                _diff_frame = _current_frame - _last_frame;
              }
            }
            _last_frame = _current_frame;
          }
          _task->call(this);
        }
      }
    private:
      Task*       _task;
      Driver&     _driver;
      Server&     _server;
      bool        _disable;
      bool        _running;
      AT_64       _current_frame;
      AT_64       _last_frame;
      AT_64       _diff_frame;
      AT_64       _clock_rate;
      unsigned    _num_diff;
      unsigned    _frame_sz;
      char*       _buffer;
      uint16_t*   _frame_ptr;
      uint64_t*   _framenum_ptr;
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
      static const unsigned fid_rollover_secs = Pds::TimeStamp::MaxFiducials / 360;
      L1Action(Manager& mgr) :
        _mgr(mgr),
        _lreset(true),
        _synced(true),
        _sync_msg(true),
        _unsycned_count(0),
        _dgm_ts(0),
        _nfid(0),
        _cam_ts(0),
        _occPool(sizeof(UserMessage),4) {}
      ~L1Action() {}
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
      void set_clock_freq(double clock_freq) { _cam_clock_freq = clock_freq; };
      uint64_t diff_ts(uint64_t t1, uint64_t t2) {
        if (t1 > t2)
          return t1-t2;
        else
          return t2-t1;
      }
      int process(Xtc* xtc) {
        if (xtc->contains.id()==TypeId::Id_Xtc)
          iterate(xtc);
        else if (xtc->contains.value() == _zylaDataType.value()) {
          ZylaDataType* frame = reinterpret_cast<ZylaDataType*>(xtc->payload());

          // vimba sdk sometimes marks frames as damaged so add those user bits to the L1Accept datagram as well
          if (xtc->damage.value() & (1<<Pds::Damage::UserDefined)) {
            _in->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
            _in->datagram().xtc.damage.userBits(xtc->damage.userBits());
          }

          if (_lreset) {
            _lreset = false;
            _dgm_ts   = _in->datagram().seq.stamp().fiducials();
            _dgm_time = _in->datagram().seq.clock();
            // reset camera timestamp
            _cam_ts = frame->timestamp();
          } else {
            const double clkratio  = 360./_cam_clock_freq;
            const double tolerance = 0.0055; // AC line rate jitter and clock drift of the zyla internal clock
            const unsigned maxdfid = 21600;  // if there is more than 1 minute between triggers
            const unsigned maxunsync = 240;

            double fdelta = double(frame->timestamp() - _cam_ts)*clkratio/double(_nfid) - 1;
            if (fabs(fdelta) > tolerance && (_nfid < maxdfid || !_synced)) {
              unsigned nfid = unsigned(double(frame->timestamp() - _cam_ts)*clkratio + 0.5);
              printf("  timestep error: fdelta %f  dfid %d  tds %lu,%lu [%d]\n", fdelta, _nfid, frame->timestamp(), _cam_ts, nfid);
              _synced = false;
            } else {
              _synced = true;
            }

            if (!_synced) {
              xtc->damage.increase(Pds::Damage::OutOfSynch);
              // add the damage to the L1Accept datagram as well
              _in->datagram().xtc.damage.increase(xtc->damage.value());
              _unsycned_count++;
              if ((_unsycned_count > maxunsync) && _sync_msg) {
                printf("L1Action: Detector has been out of sync for %u frames - reconfiguration needed!\n", _unsycned_count);
                UserMessage* msg = new (&_occPool) UserMessage("Zyla Error: Detector is out of sync - reconfiguration needed!\n");
                _mgr.appliance().post(msg);
                Occurrence* occ = new(&_occPool) Occurrence(OccurrenceId::ClearReadout);
                _mgr.appliance().post(occ);
                _sync_msg = false;
              }
            } else {
              _unsycned_count = 0;
              _dgm_ts   = _in->datagram().seq.stamp().fiducials();
              _dgm_time = _in->datagram().seq.clock();
              _cam_ts = frame->timestamp();
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
      unsigned    _dgm_ts;
      unsigned    _nfid;
      uint64_t    _cam_ts;
      double      _cam_clock_freq;
      ClockTime   _dgm_time;
      GenericPool _occPool;
      InDatagram* _in;
    };


    class ConfigAction : public Action {
    public:
      enum { MaxErrMsgLength=256 };
      ConfigAction(Manager& mgr, Driver& driver, Server& server,
                   FrameReader& reader, ConfigCache& cfg, L1Action& l1) :
        _mgr(mgr),
        _driver(driver),
        _server(server),
        _reader(reader),
        _cfg(cfg),
        _l1(l1),
        _occPool(sizeof(UserMessage),1) {}
      ~ConfigAction() {}
      InDatagram* fire(InDatagram* dg) {
        _cfg.record(dg);
        return dg;
      }
      Transition* fire(Transition* tr) {

        int len = _cfg.fetch(tr);

        if (len <= 0) {
          fprintf(stderr,
                  "ConfigAction: failed to retrieve configuration: (%d) %s.\n",
                  errno, strerror(errno));

          snprintf(_err_buffer,
                   MaxErrMsgLength,
                   "Zyla Config: failed to retrieve configuration for %s !",
                   DetInfo::name(static_cast<const DetInfo&>(_server.client())));
          UserMessage* msg = new (&_occPool) UserMessage(_err_buffer);
          _mgr.appliance().post(msg);
        } else {
#ifdef TIMING_DEBUG
          clock_gettime(CLOCK_REALTIME, &_time_start); // start timing configure
#endif
          _server.resetCount();
          if (_cfg.scanning()) {
            // update the configuration objet in the transition (if needed)
            _cfg.configure(false);
          } else {
            if (_cfg.configure()) {
              _server.set_frame_sz(_driver.frame_size() * sizeof(uint16_t));
              _reader.reset_diff();
              _reader.set_clock_rate(_driver.clock_rate());
              _reader.set_frame_sz(_driver.frame_size() * sizeof(uint16_t));
              _l1.reset();
              _l1.set_clock_freq(_driver.clock_rate());
            } else {
              fprintf(stderr,
                      "ConfigAction: Configuration of the detector failed!\n");
              UserMessage* msg = new (&_occPool) UserMessage(_cfg.get_error());
              _mgr.appliance().post(msg);
            }
          }
#ifdef TIMING_DEBUG
          clock_gettime(CLOCK_REALTIME, &_time_end); // end timing configure
          printf("Camera configuration completed in %6.1f ms\n",
                 TIME_DIFF(_time_start, _time_end));
#endif
        }

        return tr;
      }
    private:
      Manager&            _mgr;
      Driver&             _driver;
      Server&             _server;
      FrameReader&        _reader;
      ConfigCache&        _cfg;
      L1Action&           _l1;
      GenericPool         _occPool;
      char                _err_buffer[MaxErrMsgLength];
#ifdef TIMING_DEBUG
      timespec            _time_start;
      timespec            _time_end;
#endif
    };

    class EnableAction : public Action {
    public:
      EnableAction(Manager& mgr, Driver& driver, FrameReader& reader, bool wait_cooling):
        _mgr(mgr),
        _driver(driver),
        _reader(reader),
        _occPool(sizeof(UserMessage),1),
        _error(false),
        _wait_cooling(wait_cooling) { }
      ~EnableAction() { }
      InDatagram* fire(InDatagram* dg) {
        if (_error) {
          fprintf(stderr, "EnableAction: failed to enable Zyla.\n");
          dg->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
        }
        return dg;
      }
      Transition* fire(Transition* tr) {
#ifdef TIMING_DEBUG
        clock_gettime(CLOCK_REALTIME, &_time_start); // start timing enable
#endif
        if (_driver.cooling_on()) {
          _driver.get_cooling_status(_wc_buffer, AT_MAX_MSG_LEN);
          printf("Current cooling status (temp): %ls (%.2f C)\n", _wc_buffer, _driver.temperature());
          if (!_driver.check_cooling()) {
            UserMessage* msg = new (&_occPool) UserMessage("Zyla: camera cooling has not yet stablized at the setpoint!");
            _mgr.appliance().post(msg);
            if (_wait_cooling) {
              fprintf(stderr, "EnableAction: cooling is not ready!\n");
              _error = true;
              return tr;
            }
          }
        }
        _error = !_driver.start();
        if (!_error) _reader.enable();
#ifdef TIMING_DEBUG
        clock_gettime(CLOCK_REALTIME, &_time_end); // end timing enable
        printf("Camera start acquistion completed in %6.1f ms\n", TIME_DIFF(_time_start, _time_end));
#endif
        return tr;
      }
    private:
      Manager&      _mgr;
      Driver&       _driver;
      FrameReader&  _reader;
      GenericPool   _occPool;
      bool          _error;
      bool          _wait_cooling;
      AT_WC         _wc_buffer[AT_MAX_MSG_LEN];
#ifdef TIMING_DEBUG
      timespec      _time_start;
      timespec      _time_end;
#endif
    };

    class DisableAction : public Action {
    public:
      DisableAction(Driver& driver, FrameReader& reader): _driver(driver), _reader(reader), _error(false) { }
      ~DisableAction() { }
      InDatagram* fire(InDatagram* dg) {
        if (_error) {
          fprintf(stderr, "DisableAction: failed to disable Zyla.\n");
          dg->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
        }
        return dg;
      }
      Transition* fire(Transition* tr) {
#ifdef TIMING_DEBUG
        clock_gettime(CLOCK_REALTIME, &_time_start); // start timing disable
#endif
        _error = !_driver.stop(false);
        _reader.disable();
#ifdef TIMING_DEBUG
        clock_gettime(CLOCK_REALTIME, &_time_end); // end timing disable
        printf("Camera stop acquistion completed in %6.1f ms\n", TIME_DIFF(_time_start, _time_end));
#endif
        return tr;
      }
    private:
      Driver&       _driver;
      FrameReader&  _reader;
      bool          _error;
#ifdef TIMING_DEBUG
      timespec      _time_start;
      timespec      _time_end;
#endif
    };

    class BeginCalibCycleAction : public Action {
    public:
      BeginCalibCycleAction(Driver& driver, Server& server, FrameReader& reader,
                            ConfigCache& cfg, L1Action& l1) :
        _driver(driver),
        _server(server),
        _reader(reader),
        _cfg(cfg),
        _l1(l1) {}
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
            printf("BeginCalibCycleAction: failed to configure Zyla during scan!\n");
          } else {
            _server.set_frame_sz(_driver.frame_size() * sizeof(uint16_t));
            _reader.reset_diff();
            _reader.set_clock_rate(_driver.clock_rate());
            _reader.set_frame_sz(_driver.frame_size() * sizeof(uint16_t));
            _l1.reset();
            _l1.set_clock_freq(_driver.clock_rate());
          }
        }

        return tr;
      }
    private:
      Driver&       _driver;
      Server&       _server;
      FrameReader&  _reader;
      ConfigCache&  _cfg;
      L1Action&     _l1;
    };

    class EndCalibCycleAction : public Action {
    public:
      EndCalibCycleAction(ConfigCache& cfg):
        _cfg(cfg) {}
      ~EndCalibCycleAction() {}
      InDatagram* fire(InDatagram* dg) {
        return dg;
      }
      Transition* fire(Transition* tr) {
        _cfg.next();
        return tr;
      }
    private:
      ConfigCache&  _cfg;
    };
  }
}

using namespace Pds::Zyla;

Manager::Manager(Driver& driver, Server& server,
                 ConfigCache& cfg, bool wait_cooling) :
  _fsm(*new Pds::Fsm())
{
  Task* task = new Task(TaskObject("ZylaReadout",35));
  FrameReader& reader = *new FrameReader(driver, server,task);

  L1Action* l1 = new L1Action(*this);

  _fsm.callback(Pds::TransitionId::Map,
                new AllocAction(cfg));
  _fsm.callback(Pds::TransitionId::Configure,
                new ConfigAction(*this, driver, server, reader, cfg, *l1));
  _fsm.callback(Pds::TransitionId::Enable,
                new EnableAction(*this, driver, reader, wait_cooling));
  _fsm.callback(Pds::TransitionId::Disable,
                new DisableAction(driver, reader));
  _fsm.callback(Pds::TransitionId::BeginCalibCycle,
                new BeginCalibCycleAction(driver, server, reader, cfg, *l1));
  _fsm.callback(Pds::TransitionId::EndCalibCycle,
                new EndCalibCycleAction(cfg));
  _fsm.callback(Pds::TransitionId::L1Accept,
                l1);
}

Manager::~Manager() {}

Pds::Appliance& Manager::appliance() {return _fsm;}

#undef AT_MAX_MSG_LEN
#undef TIME_DIFF
#undef TIMING_DEBUG
