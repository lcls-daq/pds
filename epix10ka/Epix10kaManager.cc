/*
 * Epix10kaManager.cc
 *
 *  Created on: 2017.10.27
 *      Author: jackp
 */

#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <new>
#include <errno.h>
#include <math.h>

#include "pds/xtc/CDatagram.hh"
#include "pds/client/Fsm.hh"
#include "pds/client/Action.hh"
#include "pds/config/EpixConfigType.hh"
#include "pds/epix10ka/Epix10kaServer.hh"
#include "pds/pgp/DataImportFrame.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pds/epix10ka/Epix10kaManager.hh"
#include "pds/config/CfgClientNfs.hh"
#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/Damage.hh"
#include "pds/config/PgpCfgCache.hh"
#include "pds/utility/Occurrence.hh"
#include "pds/service/GenericPool.hh"

namespace Pds {
  class Allocation;
  class Transition;
  class InDatagram;


  class Epix10kaConfigCache : public PgpCfgCache {
    public:
      enum {StaticALlocationNumberOfConfigurationsForScanning=100};
      Epix10kaConfigCache(const Src& src) :
        PgpCfgCache(src, _epix10kaConfigType, StaticALlocationNumberOfConfigurationsForScanning * __size()),
        _cache(0) {}
      virtual ~Epix10kaConfigCache() {
        if (_cache)
          delete[] _cache;
      }
    public:
      void printCurrent() {
        Epix10kaConfigType* cfg = (Epix10kaConfigType*)current();
        printf("Epix10kaConfigCache::printCurrent current 0x%x\n", (unsigned) (unsigned long)cfg);
      }

      void record(InDatagram* dg) {
        if (_changed) {
          if (_configtc.damage.value())
            dg->datagram().xtc.damage.increase(_configtc.damage.value());
          else {
            //  Record configuration as it was readback
            dg->insert(_configtc, _cache);
          }
        }
      }

      unsigned configure(Pds::Epix10kaServer* srv) {
        const Epix10kaConfigType* cfg = reinterpret_cast<const Epix10kaConfigType*>(current());
        if (_cache) delete[] _cache;
        _cache = new char[cfg->_sizeof()];
        memcpy(_cache,cfg,cfg->_sizeof());
        return srv->configure(reinterpret_cast<Epix10kaConfigType*>(_cache));
      }
    private:
      int _size(void* tc) const { return ((Epix10kaConfigType*)tc)->_sizeof(); }
      static int __size() {
        Epix10kaConfigType* foo = new Epix10kaConfigType(2,2,0x160, 0x180, 2);
        int size = (int) foo->_sizeof();
        delete foo;
        return size;
      }
    private:
      char* _cache;
 };
}


using namespace Pds;

class Epix10kaAllocAction : public Action {

  public:
   Epix10kaAllocAction(Epix10kaConfigCache& cfg, Epix10kaServer* svr)
      : _cfg(cfg), _svr(svr) {
   }

   Transition* fire(Transition* tr)
   {
      const Allocate& alloc = reinterpret_cast<const Allocate&>(*tr);
      _cfg.init(alloc.allocation());
      _svr->allocated(alloc);
      return tr;
   }

 private:
   Epix10kaConfigCache& _cfg;
   Epix10kaServer*      _svr;
};

class Epix10kaUnmapAction : public Action {
  public:
    Epix10kaUnmapAction(Epix10kaServer* s) : server(s) {};

    Transition* fire(Transition* tr) {
      printf("Epix10kaUnmapAction::fire(tr)\n");
      return tr;
    }

  private:
    Epix10kaServer* server;
};

class Epix10kaL1Action : public Action {
 public:
   Epix10kaL1Action(Epix10kaServer* svr);

   InDatagram* fire(InDatagram* in);

   Epix10kaServer* server;
   unsigned _lastMatchedFiducial;
//   unsigned _ioIndex;
   bool     _fiducialError;

};

Epix10kaL1Action::Epix10kaL1Action(Epix10kaServer* svr) :
    server(svr),
    _lastMatchedFiducial(0xfffffff),
    _fiducialError(false) {}

InDatagram* Epix10kaL1Action::fire(InDatagram* in) {
  if (server->debug() & 8) printf("Epix10kaL1Action::fire!\n");
  if (in->datagram().xtc.damage.value() == 0) {
//    Pds::Pgp::DataImportFrame* data;
    Datagram& dg = in->datagram();
    Xtc* xtc = &(dg.xtc);
//    unsigned evrFiducials = dg.seq.stamp().fiducials();
    unsigned error = 0;
    char*    payload;
    if (xtc->contains.id() == Pds::TypeId::Id_Xtc) {
      xtc = (Xtc*) xtc->payload();
      if (xtc->contains.id() == Pds::TypeId::Id_Xtc) {
        payload = xtc->payload();
      } else {
        printf("Epix10kaL1Action::fire inner xtc not Id_EpixElement, but %s!\n",
            xtc->contains.name(xtc->contains.id()));
        return in;
      }
    } else {
      printf("Epix10kaL1Action::fire outer xtc not Id_Xtc, but %s!\n",
          xtc->contains.name(xtc->contains.id()));
      return in;
    }

    if (error) {
      dg.xtc.damage.increase(Pds::Damage::UserDefined);
      dg.xtc.damage.userBits(0xf0 | (error&0xf));
      printf("Epix10kaL1Action setting user damage due to fiducial in quads(0x%x)\n", error);
      if (!_fiducialError) server->printHisto(false);
      else _fiducialError = true;
    } else {
//      server->process();
    }
  }
  return in;
}

class Epix10kaConfigAction : public Action {

  public:
    Epix10kaConfigAction( Pds::Epix10kaConfigCache& cfg, Epix10kaServer* server)
      : _cfg( cfg ), _server(server),
      _occPool(new GenericPool(sizeof(UserMessage),4)), _result(0)
      {}

    ~Epix10kaConfigAction() {}

    Transition* fire(Transition* tr) {
      _result = 0;
      int i = _cfg.fetch(tr);
      printf("Epix10kaConfigAction::fire(Transition) fetched %d\n", i);
      _server->resetOffset();
      if (_cfg.scanning() == false) {
        if ((_result = _cfg.configure(_server))) {
          printf("\nEpix10kaConfigAction::fire(tr) failed configuration\n");
        };
        if (_server->debug() & 0x10) _cfg.printCurrent();
      } else {
        _server->offset(0);
      }
      return tr;
    }

    InDatagram* fire(InDatagram* in) {
      printf("Epix10kaConfigAction::fire(InDatagram) recorded\n");
      _cfg.record(in);
      if (_server->samplerConfig()) {
        printf("Epix10kaConfigAction sending sampler config\n");
        in->insert(_server->xtcConfig(), _server->samplerConfig());
      } else {
        printf("Epix10kaConfigAction not sending sampler config\n");
      }
      if (_server->debug() & 0x10) _cfg.printCurrent();
      if( _result ) {
        printf( "*** Epix10kaConfigAction found configuration errors _result(0x%x)\n", _result );
        if (in->datagram().xtc.damage.value() == 0) {
          in->datagram().xtc.damage.increase(Damage::UserDefined);
          in->datagram().xtc.damage.userBits(_result);
        }
        char message[400];
        sprintf(message, "Epix10ka  on host %s failed to configure!\n", getenv("HOSTNAME"));
        UserMessage* umsg = new (_occPool) UserMessage;
        umsg->append(message);
        umsg->append(DetInfo::name(static_cast<const DetInfo&>(_server->xtc().src)));
        umsg->append("\n");
        umsg->append(_server->configurator()->pgp()->errorString());
        printf("Epix10kaConfigAction %s %s", message, _server->configurator()->pgp()->errorString());
        _server->configurator()->pgp()->clearErrorString();
        _server->manager()->appliance().post(umsg);
      }
      return in;
    }

  private:
    Epix10kaConfigCache& _cfg;
    Epix10kaServer*      _server;
    GenericPool*         _occPool;
    unsigned             _result;
};

class Epix10kaBeginCalibCycleAction : public Action {
  public:
    Epix10kaBeginCalibCycleAction(Epix10kaServer* s, Epix10kaConfigCache& cfg)
     : _server(s), _cfg(cfg),_occPool(new GenericPool(sizeof(UserMessage),4)), _result(0)
     {}

    Transition* fire(Transition* tr) {
      printf("Epix10kaBeginCalibCycleAction:;fire(tr) ");
      if (_cfg.scanning()) {
        if (_cfg.changed()) {
          printf("configured and ");
          _server->offset(_server->offset()+_server->myCount()+1);
          printf(" offset %u count %u\n", _server->offset(), _server->myCount());
          if ((_result = _cfg.configure(_server))) {
            printf("\nEpix10kaBeginCalibCycleAction::fire(tr) failed config\n");
          };
          if (_server->debug() & 0x10) _cfg.printCurrent();
        }
      }
      printf("Epix10kaBeginCalibCycleAction:;fire(tr) enabled\n");
      _server->enable();
      return tr;
    }

    InDatagram* fire(InDatagram* in) {
      printf("Epix10kaBeginCalibCycleAction:;fire(InDatagram)");
      if (_cfg.scanning() && _cfg.changed()) {
        printf(" recorded\n");
        _cfg.record(in);
        if (_server->samplerConfig()) {
          printf("Epix10kaBeginCalibCycleAction sending sampler config\n");
          in->insert(_server->xtcConfig(), _server->samplerConfig());
        } else {
          printf("Epix10kaBeginCalibCycleAction not sending sampler config\n");
        }
        if (_server->debug() & 0x10) _cfg.printCurrent();
      } else printf("\n");
      if( _result ) {
        printf( "*** Epix10kaConfigAction found configuration errors _result(0x%x)\n", _result );
        if (in->datagram().xtc.damage.value() == 0) {
          in->datagram().xtc.damage.increase(Damage::UserDefined);
          in->datagram().xtc.damage.userBits(_result);
        }
        char message[400];
        sprintf(message, "Epix10ka  on host %s failed to configure!\n", getenv("HOSTNAME"));
        UserMessage* umsg = new (_occPool) UserMessage;
        umsg->append(message);
        umsg->append(DetInfo::name(static_cast<const DetInfo&>(_server->xtc().src)));
        umsg->append("\n");
        umsg->append(_server->configurator()->pgp()->errorString());
        printf("Epix10kaBeginCalibCycleAction %s %s", message, _server->configurator()->pgp()->errorString());
        _server->configurator()->pgp()->clearErrorString();
        _server->manager()->appliance().post(umsg);
      }
      return in;
    }
  private:
    Epix10kaServer*       _server;
    Epix10kaConfigCache&  _cfg;
    GenericPool*          _occPool;
    unsigned              _result;
};


class Epix10kaUnconfigAction : public Action {
 public:
   Epix10kaUnconfigAction( Epix10kaServer* server, Epix10kaConfigCache& cfg ) : _server( server ), _cfg(cfg), _result(0) {}
   ~Epix10kaUnconfigAction() {}

   Transition* fire(Transition* tr) {
     printf("Epix10kaUnconfigAction:;fire(Transition) unconfigured\n");
     _result = _server->unconfigure();
     return tr;
   }

   InDatagram* fire(InDatagram* in) {
     printf("Epix10kaUnconfigAction:;fire(InDatagram)\n");
      if( _result ) {
         printf( "*** Found %d epix10ka Unconfig errors\n", _result );
         in->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
         in->datagram().xtc.damage.userBits(0xda);
      }
      return in;
   }

 private:
   Epix10kaServer*      _server;
   Epix10kaConfigCache& _cfg;
   unsigned         _result;
};

class Epix10kaEndCalibCycleAction : public Action {
  public:
    Epix10kaEndCalibCycleAction(Epix10kaServer* s, Epix10kaConfigCache& cfg) : _server(s), _cfg(cfg), _result(0) {};

    Transition* fire(Transition* tr) {
      printf("Epix10kaEndCalibCycleAction:;fire(Transition) %p", _cfg.current());
      _cfg.next();
      printf(" %p\n", _cfg.current());

      return tr;
    }

    InDatagram* fire(InDatagram* in) {
      printf("Epix10kaEndCalibCycleAction:;fire(InDatagram)\n");
      if( _result ) {
        printf( "*** Epix10kaEndCalibCycleAction found unconfigure errors _result(0x%x)\n", _result );
        if (in->datagram().xtc.damage.value() == 0) {
          in->datagram().xtc.damage.increase(Damage::UserDefined);
          in->datagram().xtc.damage.userBits(_result);
        }
      }
      _server->disable();
      return in;
    }

  private:
    Epix10kaServer*      _server;
    Epix10kaConfigCache& _cfg;
    unsigned          _result;
};

Epix10kaManager::Epix10kaManager( Epix10kaServer* server, unsigned d) :
    _fsm(*new Fsm), _cfg(*new Epix10kaConfigCache(server->client())) {

   printf("Epix10kaManager being initialized... " );

   server->manager(this);

   _fsm.callback( TransitionId::Map, new Epix10kaAllocAction( _cfg, server ) );
   _fsm.callback( TransitionId::Unmap, new Epix10kaUnmapAction( server ) );
   _fsm.callback( TransitionId::Configure, new Epix10kaConfigAction(_cfg, server ) );
   //   _fsm.callback( TransitionId::Enable, new Epix10kaEnableAction( server ) );
   //   _fsm.callback( TransitionId::Disable, new Epix10kaDisableAction( server ) );
   _fsm.callback( TransitionId::BeginCalibCycle, new Epix10kaBeginCalibCycleAction( server, _cfg ) );
   _fsm.callback( TransitionId::EndCalibCycle, new Epix10kaEndCalibCycleAction( server, _cfg ) );
  _fsm.callback( TransitionId::L1Accept, new Epix10kaL1Action( server ) );
   _fsm.callback( TransitionId::Unconfigure, new Epix10kaUnconfigAction( server, _cfg ) );
   // _fsm.callback( TransitionId::BeginRun,
   //                new Epix10kaBeginRunAction( server ) );
   // _fsm.callback( TransitionId::EndRun,
   //                new Epix10kaEndRunAction( server ) );
}
