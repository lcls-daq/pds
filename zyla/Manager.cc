#include "pds/zyla/Manager.hh"
#include "pds/zyla/Driver.hh"
#include "pds/zyla/Server.hh"

#include "pds/config/ZylaConfigType.hh"
#include "pds/config/ZylaDataType.hh"
#include "pds/config/CfgClientNfs.hh"
#include "pds/epicstools/EpicsCA.hh"
#include "pds/client/Action.hh"
#include "pds/client/Fsm.hh"
#include "pds/utility/Appliance.hh"
#include "pds/service/GenericPool.hh"
#include "pds/service/Task.hh"
#include "pdsdata/xtc/XtcIterator.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include <errno.h>
#include <string>

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
            _server.post((char*) _buffer, sizeof(ZylaDataType) + _frame_sz);
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
      AllocAction(CfgClientNfs& cfg) : _cfg(cfg) {}
      Transition* fire(Transition* tr) {
        const Allocate& alloc = reinterpret_cast<const Allocate&>(*tr);
        _cfg.initialize(alloc.allocation());
        return tr;
      }
    private:
      CfgClientNfs& _cfg;
    };

#define PV_NAME(base,var) (std::string(base)+":"+std::string(var)).c_str()

    class IstarConfig {
    public:
      IstarConfig(const char* base) :
        _filename   (0),
        _pv_gatemode(0),
        _pv_mcpgain (0)
      {
        if (base[0]=='/')
          _filename = base;
        else {
          _pv_gatemode = new Pds_Epics::EpicsCA(PV_NAME(base,"GateMode"),0);
          _pv_mcpgain  = new Pds_Epics::EpicsCA(PV_NAME(base,"MCPGain" ),0);
        }
      }
      ~IstarConfig()
      {
        if (!_filename) {
          delete _pv_gatemode;
          delete _pv_mcpgain;
        }
      }
    public:
      bool configure(Driver& driver)
      {
        Driver::GateMode gatemode;
        int mcpgain;
        if (_filename) {
          FILE* f = fopen(_filename,"r");
          int igm;
          if (!f || fscanf(f,"%d %d", &igm, &mcpgain)!=2)
            return false;
          fclose(f);
          gatemode = Driver::GateMode(igm);
        }
        else {
          if (!_pv_gatemode->connected() || !_pv_mcpgain->connected()) {
            fprintf(stderr, "ConfigAction: failed to connect to ISTAR PVs\n");
            return false;
          }
          gatemode = Driver::GateMode(*(int*)_pv_gatemode->data());
          mcpgain  = *(int*)_pv_mcpgain->data();
        }
        printf("Setting gatemode %u and mcpgain %u\n", gatemode, mcpgain);
        if (!driver.set_gate_mode(gatemode) ||
            !driver.set_mcp_gain (mcpgain)) {
          fprintf(stderr, "ConfigAction: failed to set ISTAR config options\n");
          return false;
        }
        return true;
      }
    private:
      const char* _filename;
      Pds_Epics::EpicsCA* _pv_gatemode;
      Pds_Epics::EpicsCA* _pv_mcpgain;
    };

    class ConfigAction : public Action {
    public:
      enum { MaxErrMsgLength=256 };
      ConfigAction(Manager& mgr, Driver& driver, Server& server, FrameReader& reader, CfgClientNfs& cfg, char* istaropt) :
        _mgr(mgr),
        _driver(driver),
        _server(server),
        _reader(reader),
        _cfg(cfg),
        _cfgtc(_zylaConfigType,cfg.src()),
        _occPool(sizeof(UserMessage),1),
        _error(false),
        _istar(istaropt ? new IstarConfig(istaropt):0)
      {
      }
      ~ConfigAction() 
      {
        if (_istar) delete _istar;
      }
      InDatagram* fire(InDatagram* dg) {
        if (_error) {
          printf("*** Found configuration errors\n");
          dg->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
        } else {
          // insert assumes we have enough space in the input datagram
          dg->insert(_cfgtc,    &_config);
        }
        return dg;
      }
      Transition* fire(Transition* tr) {
        _error = false;

        int len = _cfg.fetch( *tr, _zylaConfigType, &_config, sizeof(_config) );

        if (len <= 0) {
          _error = true;

          fprintf(stderr, "ConfigAction: failed to retrieve configuration: (%d) %s.\n", errno, strerror(errno));

          snprintf(_err_buffer,
                   MaxErrMsgLength,
                   "Zyla Config: failed to retrieve configuration for %s !",
                   DetInfo::name(static_cast<const DetInfo&>(_cfgtc.src)));
          UserMessage* msg = new (&_occPool) UserMessage(_err_buffer);
          _mgr.appliance().post(msg);
        } else {
          _cfgtc.extent = sizeof(Xtc) + sizeof(ZylaConfigType);

#ifdef TIMING_DEBUG
          clock_gettime(CLOCK_REALTIME, &_time_start); // start timing configure
#endif
          if (_istar && !_istar->configure(_driver)) {
            _error = true;
            snprintf(_err_buffer,
                     MaxErrMsgLength,
                     "Zyla Config: failed to connect configure ISTAR options %s !",
                     DetInfo::name(static_cast<const DetInfo&>(_cfgtc.src)));
            UserMessage* msg = new (&_occPool) UserMessage(_err_buffer);
            _mgr.appliance().post(msg);
            return tr;
          }

          if (!_driver.set_image(_config.width(),
                                 _config.height(),
                                 _config.orgX(),
                                 _config.orgY(),
                                 _config.binX(),
                                 _config.binY(),
                                 _config.noiseFilter(),
                                 _config.blemishCorrection())) {
            _error = true;
            fprintf(stderr, "ConfigAction: failed to apply image/ROI configuration.\n");

            snprintf(_err_buffer,
                     MaxErrMsgLength,
                     "Zyla Config: failed to apply image/ROI configuration for %s !",
                     DetInfo::name(static_cast<const DetInfo&>(_cfgtc.src)));
            UserMessage* msg = new (&_occPool) UserMessage(_err_buffer);
            _mgr.appliance().post(msg);

            return tr;
          }

          if (!_driver.set_readout(_config.shutter(), _config.readoutRate(), _config.gainMode())) {
            _error = true;
            fprintf(stderr, "ConfigAction: failed to apply readout configuration.\n");
            snprintf(_err_buffer,
                     MaxErrMsgLength,
                     "Zyla Config: failed to apply readout configuration for %s !",
                     DetInfo::name(static_cast<const DetInfo&>(_cfgtc.src)));
            UserMessage* msg = new (&_occPool) UserMessage(_err_buffer);
            _mgr.appliance().post(msg);

            return tr;
          }

          // Test if a supported trigger mode for the camera has been chosen
          if (_config.triggerMode() != ZylaConfigType::ExternalExposure && _config.triggerMode() != ZylaConfigType::External) {
            _error = true;
            fprintf(stderr, "ConfigAction: unsupported trigger configuration mode: %d.\n", _config.triggerMode());
            snprintf(_err_buffer,
                     MaxErrMsgLength,
                     "Zyla Config: unsupported trigger configuration mode for %s !",
                     DetInfo::name(static_cast<const DetInfo&>(_cfgtc.src)));
            UserMessage* msg = new (&_occPool) UserMessage(_err_buffer);
            _mgr.appliance().post(msg);

            return tr;
          }

          if (!_driver.set_trigger(_config.triggerMode(), _config.triggerDelay(), _config.overlap())) {
            _error = true;
            fprintf(stderr, "ConfigAction: failed to apply trigger configuration.\n");
            snprintf(_err_buffer,
                     MaxErrMsgLength,
                     "Zyla Config: failed to apply trigger configuration for %s !",
                     DetInfo::name(static_cast<const DetInfo&>(_cfgtc.src)));
            UserMessage* msg = new (&_occPool) UserMessage(_err_buffer);
            _mgr.appliance().post(msg);

            return tr;
          }

          if (!_driver.set_exposure(_config.exposureTime())) {
            _error = true;
            fprintf(stderr, "ConfigAction: failed to apply exposure time configuration.\n");
            fprintf(stderr, "ConfigAction: Requested exposure of %.5f s is outside the allowed range of %.5f to %.5f s.\n",
                    _config.exposureTime(), _driver.exposure_min(), _driver.exposure_max());
            snprintf(_err_buffer,
                     MaxErrMsgLength,
                     "Zyla Config: failure setting exposure time for %s !\n"
                     "\nRequested exposure of %.5f s is outside the range of %.5f to %.5f s allowed by the current camera settings.",
                     DetInfo::name(static_cast<const DetInfo&>(_cfgtc.src)),
                     _config.exposureTime(),
                     _driver.exposure_min(),
                     _driver.exposure_max());
            UserMessage* msg = new (&_occPool) UserMessage(_err_buffer);
            if (_driver.overlap_mode()) {
              snprintf(_err_buffer, MaxErrMsgLength,
                       "\n\nCamera is in overlap readout mode!\n"
                       "Exposure time must be greater than the frame readout time of %.5f s + a bit extra in this mode.", _driver.readout_time());
              msg->append(_err_buffer);
            }
            _mgr.appliance().post(msg);

            return tr;
          }

          if (!_driver.set_cooling(_config.cooling(),
                                   _config.setpoint(),
                                   _config.fanSpeed())) {
            _error = true;
            fprintf(stderr, "ConfigAction: failed to apply cooling configuration.\n");
            snprintf(_err_buffer,
                     MaxErrMsgLength,
                     "Zyla Config: failed to apply cooling configuration for %s !",
                     DetInfo::name(static_cast<const DetInfo&>(_cfgtc.src)));
            UserMessage* msg = new (&_occPool) UserMessage(_err_buffer);
            _mgr.appliance().post(msg);

            return tr;
          }

          if (!_driver.configure()) {
            _error = true;
            fprintf(stderr, "ConfigAction: failed to apply configuration.\n");
            snprintf(_err_buffer,
                     MaxErrMsgLength,
                     "Zyla Config: failed to apply configuration for %s !",
                     DetInfo::name(static_cast<const DetInfo&>(_cfgtc.src)));
            UserMessage* msg = new (&_occPool) UserMessage(_err_buffer);
            _mgr.appliance().post(msg);

            return tr;
          }

          // Print cooling status info
          if (_driver.cooling_on()) {
            _driver.get_cooling_status(_wc_buffer, AT_MAX_MSG_LEN);
            printf("Current cooling status (temp): %ls (%.2f C)\n", _wc_buffer, _driver.temperature());
          }

          // Print other configure info
          printf("Image ROI (w,h)       : %lld, %lld\n", _driver.image_width(), _driver.image_height());
          printf("          (orgX,orgY) : %lld, %lld\n", _driver.image_orgX(), _driver.image_orgY());
          printf("          (binX,binY) : %lld, %lld\n", _driver.image_binX(), _driver.image_binY());
          printf("Image exposure time (sec) : %g\n", _driver.exposure());
          _driver.get_shutter_mode(_wc_buffer, AT_MAX_MSG_LEN);
          printf("Shutter mode: %ls\n", _wc_buffer);
          _driver.get_trigger_mode(_wc_buffer, AT_MAX_MSG_LEN);
          printf("Trigger mode: %ls\n", _wc_buffer);
          _driver.get_gain_mode(_wc_buffer, AT_MAX_MSG_LEN);
          printf("Gain mode: %ls\n", _wc_buffer);
          _driver.get_readout_rate(_wc_buffer, AT_MAX_MSG_LEN);
          printf("Pixel readout rate: %ls\n", _wc_buffer);
          if (_driver.overlap_mode()) {
            printf("Camera readout set to overlap mode!\n");
          }
          printf("Estimated readout time for the camera (sec): %g\n", _driver.readout_time());

          _server.resetCount();
          _server.set_frame_sz(_driver.frame_size() * sizeof(uint16_t));
          _reader.reset_diff();
          _reader.set_clock_rate(_driver.clock_rate());
          _reader.set_frame_sz(_driver.frame_size() * sizeof(uint16_t));

#ifdef TIMING_DEBUG
          clock_gettime(CLOCK_REALTIME, &_time_end); // end timing configure
          printf("Camera configuration completed in %6.1f ms\n", TIME_DIFF(_time_start, _time_end));
#endif
        }
        return tr;
      }
    private:
      Manager&            _mgr;
      Driver&             _driver;
      Server&             _server;
      FrameReader&        _reader;
      CfgClientNfs&       _cfg;
      ZylaConfigType      _config;
      Xtc                 _cfgtc;
      GenericPool         _occPool;
      bool                _error;
      char                _err_buffer[MaxErrMsgLength];
      AT_WC               _wc_buffer[AT_MAX_MSG_LEN];
#ifdef TIMING_DEBUG
      timespec            _time_start;
      timespec            _time_end;
#endif
      IstarConfig*        _istar;
      Pds_Epics::EpicsCA* _pv_gatemode;
      Pds_Epics::EpicsCA* _pv_mcpgain;
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
  }
}

using namespace Pds::Zyla;

Manager::Manager(Driver& driver, Server& server, CfgClientNfs& cfg, bool wait_cooling, char* pvbase) : _fsm(*new Pds::Fsm())
{
  Task* task = new Task(TaskObject("ZylaReadout",35));
  FrameReader& reader = *new FrameReader(driver, server,task);

  _fsm.callback(Pds::TransitionId::Map, new AllocAction(cfg));
  _fsm.callback(Pds::TransitionId::Configure, new ConfigAction(*this, driver, server, reader, cfg, pvbase));
  _fsm.callback(Pds::TransitionId::Enable   , new EnableAction(*this, driver, reader, wait_cooling));
  _fsm.callback(Pds::TransitionId::Disable  , new DisableAction(driver, reader));
}

Manager::~Manager() {}

Pds::Appliance& Manager::appliance() {return _fsm;}

#undef AT_MAX_MSG_LEN
#undef TIME_DIFF
#undef TIMING_DEBUG
