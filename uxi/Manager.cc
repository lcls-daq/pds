#include "pds/uxi/Manager.hh"
#include "pds/uxi/Detector.hh"
#include "pds/uxi/Server.hh"

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

namespace Pds {
  namespace Uxi {
    class FrameReader : public Routine {
    public:
      FrameReader(Detector& detector, Server& server, Task* task) :
        _task(task),
        _detector(detector),
        _server(server),
        _disable(true),
        _acq_stopped(false),
        _acq_count(0),
        _timestamp(0),
        _temp(0.0),
        _buffer_sz(0),
        _frame_sz(0),
        _entry_sz(0),
        _buffer(0),
        _frame_ptr(0)
      {}
      virtual ~FrameReader() {
        if (_buffer) delete[] _buffer;
      }
      Task& task() { return *_task; }
      void configure() {
        _detector.frame_size(&_frame_sz);
        _entry_sz = sizeof(UxiDataType) + _frame_sz;
        if (_buffer && (_buffer_sz < _entry_sz)) {
          delete[] _buffer;
          _buffer = NULL;
        }
        if (!_buffer) {
          _buffer_sz = _entry_sz;
          _buffer = new char[_buffer_sz];
        }
        _server.set_frame_sz(_frame_sz);
      }
      void unconfigure() {
        ;
      }
      void enable() {
        if (_disable) {
          _disable=false;
          _task->call(this);
        }
      }
      void disable() {
        _disable=true;
      }
      void routine() {
        if (_disable) {
          ;
        } else {
          _frame_ptr = (uint16_t*) (_buffer + sizeof(UxiDataType));
          if (_detector.get_frames(_acq_count, _frame_ptr, &_temp, &_timestamp, &_acq_stopped)) {
            new (_buffer) UxiDataType(_acq_count, _timestamp, _temp);
            _server.post(_buffer);
          } else if (!_acq_stopped) {
            fprintf(stderr, "Error: FrameReader failed to retrieve frames from Uxi receiver\n");
          }
          _task->call(this);
        }
      }
    private:
      Task*     _task;
      Detector& _detector;
      Server&   _server;
      bool      _disable;
      bool      _acq_stopped;
      uint32_t  _acq_count;
      uint32_t  _timestamp;
      double    _temp;
      unsigned  _buffer_sz;
      unsigned  _frame_sz;
      unsigned  _entry_sz;
      char*     _buffer;
      uint16_t* _frame_ptr;
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
            const double clkratio  = 360.0; // clkratio = 360/1 since the detector clock is in seconds
            const double tolerance = 1.1; // the uxi timer only has 1 second resolution
            const unsigned maxdfid = 21600; // if there is more than 1 minute between triggers
            const unsigned maxunsync = 240;

            double rate = clkratio/double(_nfid);
            double fdelta = double(data->timestamp() - _data_ts)*rate - 1;
            if (fabs(fdelta) > (tolerance * rate) && (_nfid < maxdfid || !_synced)) {
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
        _first(true),
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
          char side = 'A';
          double pots_rbv[UxiConfigNumberOfPots];
          uint32_t width_rbv, height_rbv, num_frames_rbv, num_bytes_rbv, type_rbv, acq_count;
          uint32_t ton_rbv, toff_rbv, tdel_rbv;
          uint32_t first_row, last_row, first_frame, last_frame;
          uint32_t osc_mode;
          const RoiCoord& roi_rows = _config.roiRows();
          const RoiCoord& roi_frames = _config.roiFrames();

          if (_first && !_detector.reset()) {
            printf("ConfigAction: failed to reset the detector on first config!\n");
            _error = true;
          } else if (_config.roiEnable() ?
              !_detector.set_row_roi(roi_rows.first(), roi_rows.last()) || !_detector.set_frame_roi(roi_frames.first(), roi_frames.last()) :
              !_detector.reset_roi()) {
            printf("ConfigAction: failed to configure the ROI of the detector!\n");
            _error = true;
          }

          if (_error) {
            UserMessage* msg = new (&_occPool) UserMessage("Uxi Config Error: failed to reset/configure the ROI of the detector!\n");
            _mgr.appliance().post(msg);
          } else {
            ndarray<const uint32_t, 1> time_on  = _config.timeOn();
            ndarray<const uint32_t, 1> time_off = _config.timeOff();
            ndarray<const uint32_t, 1> delay    = _config.delay();
            
            // Send the timing configuration to the detector
            for (unsigned iside=0; iside<UxiConfigNumberOfSides; iside++) {
              if (!_detector.set_timing(side+iside, time_on[iside], time_off[iside], delay[iside])) {
                printf("ConfigAction: failed to configure the timing of side %c of the detector!\n", side+iside);
                _error = true;
              }
            }

            if (_error) {
              UserMessage* msg = new (&_occPool) UserMessage("Uxi Config Error: failed to configure the timing of the detector!\n");
              _mgr.appliance().post(msg);
            } else {
              printf(" Timing:\n");
              for (unsigned iside=0; iside<UxiConfigNumberOfSides; iside++) {
                if (_detector.get_timing(side+iside, &ton_rbv, &toff_rbv, &tdel_rbv))
                  printf("  %c: %u %u %u\n", side+iside, ton_rbv, toff_rbv, tdel_rbv);
              }

              // Send the pot configuration (excluding the readonly pots)
              ndarray<const double, 1> pots = _config.pots();
              for (unsigned ipot=0; ipot<UxiConfigNumberOfPots; ipot++) {
                if (!_config.potIsReadOnly(ipot)) {
                  if (!_detector.set_pot(ipot+1, pots[ipot], _config.potIsTuned(ipot))) {
                    printf("ConfigAction: failed to configure pot %u of the detector!\n", ipot+1);
                    _error = true;
                  }
                }
              }

              if (_error) {
                UserMessage* msg = new (&_occPool) UserMessage("Uxi Config Error: failed to configure the pots of the detector!\n");
                _mgr.appliance().post(msg);
              } else {
                // Send the oscillator configuration
                if (!_detector.set_oscillator(_config.oscillator())) {
                  printf("ConfigAction: failed to set oscillator mode of the detector to %u!\n", _config.oscillator());
                  _error = true;
                }

                if (_error) {
                  UserMessage* msg = new (&_occPool) UserMessage("Uxi Config Error: failed to configure the oscillator mode the detector!\n");
                  _mgr.appliance().post(msg);
                } else {
                  if (_detector.commit()) {
                    for (unsigned ipot=0; ipot<UxiConfigNumberOfPots; ipot++) {
                      if (!_detector.get_pot(ipot+1, &pots_rbv[ipot])) {
                        printf("ConfigAction: failed to readback the value of pot %u of the detector!\n", ipot+1);
                        _error = true;
                      }
                    }

                    if (_error) {
                      UserMessage* msg = new (&_occPool) UserMessage("Uxi Config Error: failed to readback the pots of the detector!\n");
                      _mgr.appliance().post(msg);
                    } else {
                      if (!_detector.width(&width_rbv)) {
                        printf("ConfigAction: failed to readback width parametar from detector!\n");
                        _error = true;
                      } else if (!_detector.height(&height_rbv)) {
                        printf("ConfigAction: failed to readback height parameter from detector!\n");
                        _error = true;
                      } else if (!_detector.nframes(&num_frames_rbv)) {
                        printf("ConfigAction: failed to readback num_frames parameter from detector!\n");
                        _error = true;
                      } else if (!_detector.nbytes(&num_bytes_rbv)) {
                        printf("ConfigAction: failed to readback nbytes parameter from detector!\n");
                        _error = true;
                      } else if (!_detector.type(&type_rbv)) {
                        printf("ConfigAction: failed to readback type parameter from detector!\n");
                        _error = true;
                      } else if (!_detector.acq_count(&acq_count)) {
                        printf("ConfigAction: failed to readback acq_count parameter from detector!\n");
                        _error = true;
                      } else if (!_detector.get_row_roi(&first_row, &last_row)) {
                        printf("ConfigAction: failed to readback row_roi parameters from detector!\n");
                        _error = true;
                      } else if (!_detector.get_frame_roi(&first_frame, &last_frame)) {
                        printf("ConfigAction: failed to readback frame_roi parameters from detector!\n");
                        _error = true;
                      } else if (!_detector.get_oscillator(&osc_mode)) {
                        printf("ConfigAction: failed to readback oscillator mode from detector!\n");
                        _error = true;
                      }

                      if (_error) {
                        UserMessage* msg = new (&_occPool) UserMessage("Uxi Config Error: unable to readback frame configuration!\n");
                        _mgr.appliance().post(msg);
                      } else if (num_frames_rbv > _max_num_frames) { // check that the number of frames does not exceed max_num_frames
                        printf("ConfigAction: the detector has %u frames, but the event builder is only configured for %u!\n", num_frames_rbv, _max_num_frames);
                        _error = true;
                        UserMessage* msg = new (&_occPool) UserMessage("Uxi Config Error: the detector has more frames than the maximum set in the event builder!\n");
                        _mgr.appliance().post(msg);
                      } else {
                        printf("Detector info:\n");
                        printf(" Width is:      %u\n", width_rbv);
                        printf(" Height is:     %u\n", height_rbv);
                        printf(" Num frames is: %u\n", num_frames_rbv);
                        printf(" Num bytes is:  %u\n", num_bytes_rbv);
                        printf(" Type is:       %u\n", type_rbv);
                        printf(" Acq Count is:  %u\n", acq_count);
                        printf("ROI info:\n");
                        printf(" first row:     %u\n", first_row);
                        printf(" last row:      %u\n", last_row);
                        printf(" first frame:   %u\n", first_frame);
                        printf(" last frame:    %u\n", last_frame);
                        printf("Oscillator info:\n");
                        printf(" mode:          %u\n", osc_mode);
                        printf("Pots info:\n");
                        for (unsigned i=0; i<UxiConfigNumberOfPots; i++) {
                          printf(" pot%02u:     %.2f\n", i, pots_rbv[i]);  
                        }

                        // acq_count returned by the server is the number it will send with the next frame
                        // so the last frame is one less than that!
                        _server.setFrame(acq_count-1);

                        // Add the actual frame size and roi to the config
                        UxiConfig::setSize(_config, width_rbv, height_rbv, num_frames_rbv, num_bytes_rbv, type_rbv);
                        UxiConfig::setRoi(_config, first_row, last_row, first_frame, last_frame);

                        // Add the read back pots values to the config
                        UxiConfig::setPots(_config, pots_rbv);

                        // Flush any old frames that might be waiting to read from the socket
                        if (!_first) { // on the first config the flush is already done earlier so skip
                          _detector.flush();
                        }

                        // Finally configure the frame reader
                        _reader.configure();

                        // Set the first config flag to false
                        _first = false;
                      }
                    }
                  } else {
                    printf("ConfigAction: failed to commit the configuration to the detector!\n");
                    _error = true;
                    UserMessage* msg = new (&_occPool) UserMessage("Uxi Config Error: failed to commit the configuration to the detector!\n");
                    _mgr.appliance().post(msg);
                  }
                }
              }
            }
          }

          _l1.reset();
          _server.resetCount();
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
      bool                  _first;
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
      FrameReader   _reader;
      bool          _error;
    };

    class EnableAction : public Action {
    public:
      EnableAction(Detector& detector, FrameReader& reader, L1Action& l1):
        _detector(detector),
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
        _reader.enable();
        _error = !_detector.start();
        return tr;
      }
    private:
      Detector&     _detector;
      FrameReader&  _reader;
      L1Action&     _l1;
      bool          _error;
    };

    class DisableAction : public Action {
    public:
      DisableAction(Detector& detector, FrameReader& reader):
        _detector(detector),
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
        _reader.disable();
        _error = !_detector.stop();
        return tr;
      }
    private:
      Detector&     _detector;
      FrameReader&  _reader;
      bool          _error;
    };
  }
}

using namespace Pds::Uxi;

Manager::Manager(Detector& detector, Server& server, CfgClientNfs& cfg, unsigned max_num_frames) : _fsm(*new Pds::Fsm())
{
  Task* task = new Task(TaskObject("UxiReadout",35));
  L1Action* l1 = new L1Action(*this);
  FrameReader& reader = *new FrameReader(detector, server, task);

  _fsm.callback(Pds::TransitionId::Map, new AllocAction(cfg));
  _fsm.callback(Pds::TransitionId::Configure  , new ConfigAction(*this, detector, server, reader, cfg, *l1, max_num_frames));
  _fsm.callback(Pds::TransitionId::Enable     , new EnableAction(detector, reader, *l1));
  _fsm.callback(Pds::TransitionId::Disable    , new DisableAction(detector, reader));
  _fsm.callback(Pds::TransitionId::Unconfigure, new UnconfigureAction(reader));
  _fsm.callback(Pds::TransitionId::L1Accept , l1);
}

Manager::~Manager() {}

Pds::Appliance& Manager::appliance() {return _fsm;}
