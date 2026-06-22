#include "pds/nscam/Manager.hh"
#include "pds/nscam/Detector.hh"
#include "pds/nscam/Server.hh"
#include "pds/nscam/Error.hh"

#include "pds/config/UxiConfigType.hh"
#include "pds/config/UxiDataType.hh"
#include "pds/config/CfgClientNfs.hh"
#include "pds/client/Action.hh"
#include "pds/client/Fsm.hh"
#include "pds/utility/Appliance.hh"
#include "pds/service/GenericPool.hh"
#include "pds/service/Task.hh"
#include "pdsdata/xtc/XtcIterator.hh"

#include <errno.h>
#include <math.h>
#include <atomic>

static const char*    UxiConfigPotNames[] = {
  "COL_BOT_IBIAS_IN", // POT1
  "HST_A_PDELAY",     // POT2
  "HST_B_NDELAY",     // POT3
  "HST_RO_IBIAS",     // POT4
  "HST_OSC_VREF_IN",  // POT5
  "HST_B_PDELAY",     // POT6
  "HST_OSC_CTL",      // POT7
  "HST_A_NDELAY",     // POT8
  "COL_TOP_IBIAS_IN", // POT9
  "HST_OSC_R_BIAS",   // POT10
  "VAB",              // POT11
  "HST_RO_NC_IBIAS",  // POT12
  "VRST",             // POT13
};

namespace Pds {
  namespace NsCam {
    class FrameReader : public Routine {
    public:
      FrameReader(Detector& detector, Server& server, Task* task) :
        _task(task),
        _detector(detector),
        _server(server),
        _disable{true},
        _metadata{0, 0, 0.0, nullptr},
        _sem(Semaphore::FULL),
        _start_time()
      {}
      virtual ~FrameReader() {
      }
      Task& task() { return *_task; }
      void configure() {
        _metadata.acq_count = 0;
        _start_time = std::chrono::system_clock::now();
      }
      void unconfigure() {
        ;
      }
      bool enable() {
        if (_disable.exchange(false)) {
          _sem.take();
          _task->call(this);
          return true;
        } else {
          return false;
        }
      }
      bool disable() {
        if (!_disable.exchange(true)) {
          _detector.abortReadoff();
          _sem.take();
          _detector.abortReadoff(false);
          _sem.give();
          return true;
        } else {
          return false;
        }
      }
      void routine() {
        if (_disable.load()) {
          _sem.give();
        } else {
          try {
            _detector.arm();
            if (_detector.waitForSRAM()) {
              _metadata.acq_count++;
              auto current_time = std::chrono::system_clock::now();
              auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - _start_time);
              std::unique_ptr<uint16_t[]> frame = _detector.readFrame16();
              _metadata.timestamp = elapsed.count();
              _metadata.temperature = _detector.getTemp();
              _metadata.data = frame.get();
              _server.post(_metadata);
            }
          } catch (const NsCamException& err) {
            fprintf(stderr, "Error: FrameReader failed to retrieve frames from detector: %s\n", err.what());
          }
          _task->call(this);
        }
      }
    private:
      Task*            _task;
      Detector&        _detector;
      Server&          _server;
      std::atomic_bool _disable;
      FrameMetaData    _metadata;
      Semaphore        _sem;
      std::chrono::system_clock::time_point _start_time;
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

    class L1Action : public Action, public Pds::XtcIterator {
    public:
      static const unsigned skew_min_time = 0x1fffffff;
      static const unsigned fid_rollover_secs = Pds::TimeStamp::MaxFiducials / 360;
      L1Action(Manager& mgr) :
        _mgr(mgr),
        _lreset(true),
        _synced(true),
        _sync_msg(true),
        _unsycned_count(0),
        _dgm_ts(0),
        _nfid(0),
        _data_ts(0),
        _occPool(sizeof(UserMessage),4) {}
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
      int process(Xtc* xtc) {
        if (xtc->contains.id()==TypeId::Id_Xtc)
          iterate(xtc);
        else if (xtc->contains.value() == _uxiDataType.value()) {
          UxiDataType* data = (UxiDataType*) xtc->payload();

          if (_lreset) {
            _lreset = false;
            _dgm_ts   = _in->datagram().seq.stamp().fiducials();
            _dgm_time = _in->datagram().seq.clock();
            _data_ts  = data->timestamp();
          } else {
            const double clkratio  = 360.0/1000.0; // clkratio = 360/1000 since the detector clock is in milliseconds
            const double tolerance = 0.01;         // the uxi timer is based on arrival time of ready signal over the network
            const unsigned maxdfid = 21600;        // if there is more than 1 minute between triggers
            const unsigned maxunsync = 240;

            double fdelta = double(data->timestamp() - _data_ts)*clkratio/double(_nfid) - 1;
            if (fabs(fdelta) > tolerance && (_nfid < maxdfid || !_synced)) {
              unsigned nfid = unsigned(double(data->timestamp() - _data_ts)*clkratio + 0.5);
              printf("  timestep error for acq_count %u: fdelta %f  dfid %d  tds %u,%u [%d]\n",
                     data->acquisitionCount(), fdelta, _nfid, data->timestamp(), _data_ts, nfid);
              _synced = false;
            }

            if (!_synced) {
              _in->datagram().xtc.damage.increase(Pds::Damage::OutOfSynch);
              _unsycned_count++;
              if ((_unsycned_count > maxunsync) && _sync_msg) {
                printf("L1Action: Detector has been out of sync for %u frames - reconfiguration needed!\n", _unsycned_count);
                UserMessage* msg = new (&_occPool) UserMessage("Uxi Error: Detector is out of sync - reconfiguration needed!\n");
                _mgr.appliance().post(msg);
                Occurrence* occ = new(&_occPool) Occurrence(OccurrenceId::ClearReadout);
                _mgr.appliance().post(occ);
                _sync_msg = false;
              }
            } else {
              _unsycned_count = 0;
              _dgm_ts   = _in->datagram().seq.stamp().fiducials();
              _dgm_time = _in->datagram().seq.clock();
              _data_ts  = data->timestamp();
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
      uint32_t    _data_ts;
      ClockTime   _dgm_time;
      GenericPool _occPool;
      InDatagram* _in;
    };

    class ConfigAction : public Action {
    public:
      ConfigAction(Manager& mgr, Detector& detector, Server& server, FrameReader& reader, CfgClientNfs& cfg, L1Action& l1, unsigned max_num_frames) :
        _mgr(mgr),
        _detector(detector),
        _server(server),
        _cfg(cfg),
        _l1(l1),
        _reader(reader),
        _cfgtc(_uxiConfigType,cfg.src()),
        _occPool(sizeof(UserMessage),1),
        _error(false),
        _max_num_frames(max_num_frames) {}
      ~ConfigAction() {}
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

        int len = _cfg.fetch( *tr, _uxiConfigType, &_config, sizeof(_config) );

        if (len <= 0) {
          _error = true;

          printf("ConfigAction: failed to retrieve configuration: (%d) %s.\n", errno, strerror(errno));

          UserMessage* msg = new (&_occPool) UserMessage("Uxi Config Error: failed to retrieve configuration.\n");
          _mgr.appliance().post(msg);
        } else {
          _cfgtc.extent = sizeof(Xtc) + sizeof(UxiConfigType);

          try {
            if (_config.roiEnable()) {
              const Pds::Uxi::RoiCoord& roi_rows = _config.roiRows();
              const Pds::Uxi::RoiCoord& roi_frames = _config.roiFrames();
              _detector.setRows(roi_rows.first(), roi_rows.last());
              _detector.setFrames(roi_frames.first(), roi_frames.last());
            } else {
              _detector.setRows();
              _detector.setFrames();
            }
          } catch (const NsCamException& err) {
            printf("ConfigAction: failed to configure the ROI of the detector: %s\n", err.what());
            _error = true;
            UserMessage* msg = new (&_occPool) UserMessage("Uxi Config Error: failed to reset/configure the ROI of the detector!\n");
            _mgr.appliance().post(msg);
            return tr;
          }

          SideType side = SideType::AB;
          try {
            ndarray<const uint32_t, 1> time_on  = _config.timeOn();
            ndarray<const uint32_t, 1> time_off = _config.timeOff();
            ndarray<const uint32_t, 1> delay    = _config.delay();

            // Send the timing configuration to the detector
            SideType side = SideType::AB;
            for (unsigned iside=0; iside<UxiConfigNumberOfSides; iside++) {
              side = static_cast<SideType>(iside);
              _detector.setTiming(side, {time_on[iside], time_off[iside], delay[iside]});
            }
          } catch (const NsCamException& err) {
            printf("ConfigAction: failed to configure the timing of %s of the detector: %s\n", toString(side), err.what());
            _error = true;
            UserMessage* msg = new (&_occPool) UserMessage("Uxi Config Error: failed to configure the timing of the detector!\n");
            _mgr.appliance().post(msg);
            return tr;
          }

          std::string potname = "";
          try {
            // Send the pot configuration (excluding the readonly pots)
            ndarray<const double, 1> pots = _config.pots();
            for (unsigned ipot=0; ipot<UxiConfigNumberOfPots; ipot++) {
              potname = UxiConfigPotNames[ipot];
              if (!_config.potIsReadOnly(ipot)) {
                _detector.setPotV(potname, pots[ipot], _config.potIsTuned(ipot));
              }
            }
          } catch (const NsCamException& err) {
            printf("ConfigAction: failed to configure pot %s of the detector: %s\n", potname.c_str(), err.what());
            _error = true;
            UserMessage* msg = new (&_occPool) UserMessage("Uxi Config Error: failed to configure the pots of the detector!\n");
            _mgr.appliance().post(msg);
            return tr;
          }

          try {
            // Send the oscillator configuration
            _detector.setOscillator(static_cast<OscillatorType>(_config.oscillator()));
          } catch (const NsCamException& err) {
            printf("ConfigAction: failed to set oscillator mode of the detector to %u: %s\n", _config.oscillator(), err.what());
            _error = true;
            UserMessage* msg = new (&_occPool) UserMessage("Uxi Config Error: failed to configure the oscillator mode the detector!\n");
            _mgr.appliance().post(msg);
            return tr;
          }

          try {
            double pots_rbv[UxiConfigNumberOfPots];
            uint32_t width_rbv = _detector.width();
            uint32_t height_rbv = _detector.height();
            uint32_t num_frames_rbv = _detector.nframes();
            uint32_t num_bytes_rbv = _detector.bytesperpixel();
            BoardType board_type_rbv = _detector.boardType();
            SensorType sensor_type_rbv = _detector.sensorType();
            uint32_t first_row = _detector.firstrow();
            uint32_t last_row = _detector.lastrow();
            uint32_t first_frame = _detector.firstframe();
            uint32_t last_frame = _detector.lastframe();
            Timing timing_a_rbv = _detector.getTiming(SideType::A);
            Timing timing_b_rbv = _detector.getTiming(SideType::B);
            OscillatorType osc_mode = _detector.getOscillator();

            if(num_frames_rbv > _max_num_frames) { // check that the number of frames does not exceed max_num_frames
              printf("configaction: the detector has %u frames, but the event builder is only configured for %u!\n", num_frames_rbv, _max_num_frames);
              _error = true;
              UserMessage* msg = new (&_occPool) UserMessage("Uxi Config Error: the detector has more frames than the maximum set in the event builder!\n");
              _mgr.appliance().post(msg);
              return tr;
            }

            printf("Detector info:\n");
            printf(" Width is:      %u\n", width_rbv);
            printf(" Height is:     %u\n", height_rbv);
            printf(" Num frames is: %u\n", num_frames_rbv);
            printf(" Num bytes is:  %u\n", num_bytes_rbv);
            printf(" Board is:      %s\n", toString(board_type_rbv));
            printf(" Sensor is:     %s\n", toString(sensor_type_rbv));
            printf("ROI info:\n");
            printf(" first row:     %u\n", first_row);
            printf(" last row:      %u\n", last_row);
            printf(" first frame:   %u\n", first_frame);
            printf(" last frame:    %u\n", last_frame);
            printf("Oscillator info:\n");
            printf(" mode:          %s\n", toString(osc_mode));
            printf("Timing info:\n");
            printf(" side A:        %s\n", Timing::toString(timing_a_rbv).c_str());
            printf(" side B:        %s\n", Timing::toString(timing_b_rbv).c_str());
            printf("Pots info:\n");
            for (unsigned i=0; i<UxiConfigNumberOfPots; i++) {
              pots_rbv[i] = _detector.getPotV(UxiConfigPotNames[i]);
              printf(" pot%02u:     %.2f\n", i, pots_rbv[i]);  
            }

            _server.resetCount();
            _server.setFrame(0);
            _server.set_frame_sz(_detector.payloadSize());

            // Add the actual frame size and roi to the config
            UxiConfig::setSize(_config, width_rbv, height_rbv, num_frames_rbv, num_bytes_rbv, static_cast<unsigned>(sensor_type_rbv));
            UxiConfig::setRoi(_config, first_row, last_row, first_frame, last_frame);

            // Add the read back pots values to the config
            UxiConfig::setPots(_config, pots_rbv);

            // Finally configure the frame reader
            _reader.configure();

            // Reset L1Action counters
            _l1.reset();
          } catch (const NsCamException& err) {
            printf("ConfigAction: unable to readback detector configuration!\n");
            _error = true;
            UserMessage* msg = new (&_occPool) UserMessage("Uxi Config Error: unable to readback frame detector!\n");
            _mgr.appliance().post(msg);
          }
        }
        return tr;
      }
    private:
      Manager&              _mgr;
      Detector&             _detector;
      Server&               _server;
      CfgClientNfs&         _cfg;
      L1Action&             _l1;
      FrameReader&          _reader;
      UxiConfigType         _config;
      Xtc                   _cfgtc;
      GenericPool           _occPool;
      bool                  _error;
      unsigned              _max_num_frames;
    };

    class UnconfigureAction : public Action {
    public:
      UnconfigureAction(FrameReader& reader): _reader(reader), _error(false) { }
      ~UnconfigureAction() { }
      InDatagram* fire(InDatagram* dg) {
        if (_error) {
          printf("UnconfigureAction: failed to disable Jungfrau.\n");
          dg->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
        }
        return dg;
      }
      Transition* fire(Transition* tr) {
        _reader.unconfigure();
        return tr;
      }
    private:
      FrameReader&  _reader;
      bool          _error;
    };

    class EnableAction : public Action {
    public:
      EnableAction(FrameReader& reader, L1Action& l1):
        _reader(reader),
        _l1(l1),
        _error(false) { }
      ~EnableAction() { }
      InDatagram* fire(InDatagram* dg) {
        if (_error) {
          printf("EnableAction: failed to enable Uxi.\n");
          dg->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
        }
        return dg;
      }
      Transition* fire(Transition* tr) {
        _l1.sync();
        _error = !_reader.enable();
        return tr;
      }
    private:
      FrameReader&  _reader;
      L1Action&     _l1;
      bool          _error;
    };

    class DisableAction : public Action {
    public:
      DisableAction(FrameReader& reader):
        _reader(reader),
        _error(false) { }
      ~DisableAction() { }
      InDatagram* fire(InDatagram* dg) {
        if (_error) {
          printf("DisableAction: failed to disable Uxi.\n");
          dg->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
        }
        return dg;
      }
      Transition* fire(Transition* tr) {
        _error = !_reader.disable();
        return tr;
      }
    private:
      FrameReader&  _reader;
      bool          _error;
    };
  }
}

using namespace Pds::NsCam;

Manager::Manager(Detector& detector, Server& server, CfgClientNfs& cfg, unsigned max_num_frames) : _fsm(*new Pds::Fsm())
{
  Task* task = new Task(TaskObject("UxiReadout",35));
  L1Action* l1 = new L1Action(*this);
  FrameReader& reader = *new FrameReader(detector, server, task);

  _fsm.callback(Pds::TransitionId::Map, new AllocAction(cfg));
  _fsm.callback(Pds::TransitionId::Configure  , new ConfigAction(*this, detector, server, reader, cfg, *l1, max_num_frames));
  _fsm.callback(Pds::TransitionId::Enable     , new EnableAction(reader, *l1));
  _fsm.callback(Pds::TransitionId::Disable    , new DisableAction(reader));
  _fsm.callback(Pds::TransitionId::Unconfigure, new UnconfigureAction(reader));
  _fsm.callback(Pds::TransitionId::L1Accept , l1);
}

Manager::~Manager() {}

Pds::Appliance& Manager::appliance() {return _fsm;}
