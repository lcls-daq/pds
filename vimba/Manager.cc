#include "pds/vimba/Manager.hh"
#include "pds/vimba/Driver.hh"
#include "pds/vimba/Server.hh"
#include "pds/vimba/FrameBuffer.hh"
#include "pds/vimba/ConfigCache.hh"

#include "pds/config/VimbaConfigType.hh"
#include "pds/config/VimbaDataType.hh"
#include "pds/client/Action.hh"
#include "pds/client/Fsm.hh"
#include "pds/utility/Appliance.hh"
#include "pds/service/GenericPool.hh"
#include "pds/service/RingPool.hh"
#include "pdsdata/xtc/XtcIterator.hh"

#include <vector>
#include <errno.h>
#include <math.h>

namespace Pds {
  namespace Vimba {
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
      uint64_t diff_ts(uint64_t t1, uint64_t t2) {
        if (t1 > t2)
          return t1-t2;
        else
          return t2-t1;
      }
      int process(Xtc* xtc) {
        if (xtc->contains.id()==TypeId::Id_Xtc)
          iterate(xtc);
        else if (xtc->contains.value() == _vimbaDataType.value()) {
          VimbaDataType* frame = reinterpret_cast<VimbaDataType*>(xtc->payload());

          if (_lreset) {
            _lreset = false;
            _dgm_ts   = _in->datagram().seq.stamp().fiducials();
            _dgm_time = _in->datagram().seq.clock();
            // reset camera timestamp
            _cam_ts = frame->timestamp();
          } else {
            const double clkratio  = 360./1e9;
            const double tolerance = 0.005; // AC line rate jitter and clock drift
            const unsigned maxdfid = 21600; // if there is more than 1 minute between triggers
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
                UserMessage* msg = new (&_occPool) UserMessage("Vimba Error: Detector is out of sync - reconfiguration needed!\n");
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
      ClockTime   _dgm_time;
      GenericPool _occPool;
      InDatagram* _in;
    };

    class ConfigAction : public Action {
    public:
      ConfigAction(Manager& mgr, FrameBuffer& framebuf, Server& server,
                   ConfigCache& cfg, L1Action& l1) :
        _mgr(mgr),
        _framebuf(framebuf),
        _server(server),
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
          printf("ConfigAction: failed to retrieve configuration: (%d) %s.\n", errno, strerror(errno));

          UserMessage* msg = new (&_occPool) UserMessage("Vimba Config Error: failed to retrieve configuration.\n");
          _mgr.appliance().post(msg);
        } else {
          _server.resetCount();
          if (_cfg.scanning()) {
            // update the config object in the transition but don't apply to detector
            _cfg.configure(false);
          } else {
            if (_cfg.configure()) {
              _server.resetFrame();
              _l1.reset();
              _framebuf.configure();
            }
          }
        }

        return tr;
      }
    private:
      Manager&              _mgr;
      FrameBuffer&          _framebuf;
      Server&               _server;
      ConfigCache&          _cfg;
      L1Action&             _l1;
      GenericPool           _occPool;
    };

    class BeginCalibCycleAction : public Action {
    public:
      BeginCalibCycleAction(FrameBuffer& framebuf, Server& server,
                            ConfigCache& cfg, L1Action& l1):
        _framebuf(framebuf),
        _server(server),
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
            printf("BeginCalibCycleAction: failed to configure camera during scan!\n");
          } else {
            _server.resetFrame();
            _l1.reset();
            _framebuf.configure();
          }
        }

        return tr;
      }
    private:
        FrameBuffer&          _framebuf;
        Server&               _server;
        ConfigCache&          _cfg;
        L1Action&             _l1;
    };

    class EndCalibCycleAction : public Action {
    public:
      EndCalibCycleAction(FrameBuffer& framebuf, ConfigCache& cfg):
        _framebuf(framebuf),
        _cfg(cfg) {}
      ~EndCalibCycleAction() {}
      InDatagram* fire(InDatagram* dg) {
        return dg;
      }
      Transition* fire(Transition* tr) {
        _cfg.next();
        if (_cfg.scanning()) {
          _framebuf.unconfigure();
        }
        return tr;
      }
    private:
      FrameBuffer&  _framebuf;
      ConfigCache&  _cfg;
    };

    class UnconfigureAction : public Action {
    public:
      UnconfigureAction(FrameBuffer& framebuf, ConfigCache& cfg):
        _framebuf(framebuf),
        _cfg(cfg) {}
      ~UnconfigureAction() {}
      InDatagram* fire(InDatagram* dg) {
        return dg;
      }
      Transition* fire(Transition* tr) {
        if (!_cfg.scanning()) {
          _framebuf.unconfigure();
        }
        return tr;
      }
    private:
      FrameBuffer&  _framebuf;
      ConfigCache&  _cfg;
    };

    class EnableAction : public Action {
    public:
      EnableAction(FrameBuffer& framebuf, L1Action& l1): 
        _framebuf(framebuf),
        _l1(l1),
        _error(false) { }
      ~EnableAction() { }
      InDatagram* fire(InDatagram* dg) {
        if (_error) {
          printf("EnableAction: failed to enable camera.\n");
          dg->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
        }
        return dg;
      }
      Transition* fire(Transition* tr) {
        _l1.sync();
        _error = !_framebuf.enable();  
        return tr;
      }
    private:
      FrameBuffer&  _framebuf;
      L1Action&     _l1;
      bool          _error;
    };

    class DisableAction : public Action {
    public:
      DisableAction(FrameBuffer& framebuf):
        _framebuf(framebuf),
        _error(false) { }
      ~DisableAction() { }
      InDatagram* fire(InDatagram* dg) {
        if (_error) {
          printf("DisableAction: failed to disable camera.\n");
          dg->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
        }
        return dg;
      }
      Transition* fire(Transition* tr) {
        _error = !_framebuf.disable();
        return tr;
      }
    private:
      FrameBuffer&  _framebuf;
      bool          _error;
    };
  }
}

using namespace Pds::Vimba;

Manager::Manager(FrameBuffer& framebuf, Server& server, ConfigCache& cfg) :
  _fsm(*new Pds::Fsm())
{
  L1Action* l1 = new L1Action(*this);

  _fsm.callback(Pds::TransitionId::Map,             new AllocAction(cfg));
  _fsm.callback(Pds::TransitionId::Configure,       new ConfigAction(*this, framebuf, server, cfg, *l1));
  _fsm.callback(Pds::TransitionId::Enable,          new EnableAction(framebuf, *l1));
  _fsm.callback(Pds::TransitionId::Disable,         new DisableAction(framebuf));
  _fsm.callback(Pds::TransitionId::BeginCalibCycle, new BeginCalibCycleAction(framebuf, server, cfg, *l1));
  _fsm.callback(Pds::TransitionId::EndCalibCycle,   new EndCalibCycleAction(framebuf, cfg));
  _fsm.callback(Pds::TransitionId::Unconfigure,     new UnconfigureAction(framebuf, cfg));
  _fsm.callback(Pds::TransitionId::L1Accept,        l1);
}

Manager::~Manager() {}

Pds::Appliance& Manager::appliance() {return _fsm;}

