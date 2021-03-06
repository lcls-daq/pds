/*-------------------------------------------------------------------------------
-- Title      : 
-------------------------------------------------------------------------------
-- File       : Manager.cc
-- Author     : Matt Weaver <weaver@slac.stanford.edu>
-- Company    : SLAC National Accelerator Laboratory
-- Created    : 2018-09-28
-- Last update: 2018-09-17
-- Platform   : 
-- Standard   : C++
-------------------------------------------------------------------------------
-- Description: 
-------------------------------------------------------------------------------
-- This file is part of 'LCLS DAQ'.
-- It is subject to the license terms in the LICENSE.txt file found in the 
-- top-level directory of this distribution and at: 
--    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
-- No part of 'LCLS DAQ', including this file, 
-- may be copied, modified, propagated, or distributed except according to 
-- the terms contained in the LICENSE.txt file.
-------------------------------------------------------------------------------*/

#include "pds/epix10ka2m/Manager.hh"
#include "pds/config/EpixDataType.hh"

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
#include "pds/client/WorkThreads.hh"
#include "pds/config/EpixConfigType.hh"
#include "pds/epix10ka2m/ConfigCache.hh"
#include "pds/epix10ka2m/Configurator.hh"
#include "pds/epix10ka2m/Server.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pds/config/CfgClientNfs.hh"
#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/XtcIterator.hh"
#include "pdsdata/xtc/Damage.hh"
#include "pds/config/CfgCache.hh"
#include "pds/utility/Occurrence.hh"
#include "pds/service/GenericPool.hh"
#include "pds/service/GenericPoolW.hh"
#include "pds/pgp/DataImportFrame.hh"
#include "pds/pgp/Pgp.hh"

//#define DBUG

typedef std::vector<Pds::Epix10ka2m::Server*> SrvL;

namespace Pds {
  class Allocation;
  class Transition;
  class InDatagram;

  namespace Epix10ka2m {
    class AllocAction : public Action {
    public:
      AllocAction(ConfigCache& cfg, const SrvL& svr)
        : _cfg(cfg), _svr(svr) {
      }

      Transition* fire(Transition* tr)
      {
        const Allocate& alloc = reinterpret_cast<const Allocate&>(*tr);
        _cfg.init(alloc.allocation());
        for(SrvL::const_iterator it=_svr.begin(); it!=_svr.end(); it++)
          (*it)->allocated(alloc);
        return tr;
      }
    private:
      ConfigCache& _cfg;
      const SrvL&  _svr;
    };

    class UnmapAction : public Action {
    public:
      UnmapAction() {}

      Transition* fire(Transition* tr) {
        printf("%s\n",__PRETTY_FUNCTION__);
        return tr;
      }
    };

    class L1Action : public Action {
    public:
      L1Action(const ConfigCache& cfg) :
        _cfg   (cfg),
        _pool  (5<<20, 16)
      { printf("L1Action pool\n"); _pool.dump(); }
      
      InDatagram* fire(InDatagram* in);
      
    private:
      const ConfigCache& _cfg;
      GenericPool        _pool;
    };

    class ConfigAction : public Action {
    public:
      ConfigAction( ConfigCache& cfg, const SrvL& server, Pds::Appliance& app)
        : _cfg( cfg ), _server(server), _app(app),
          _occPool(new GenericPool(sizeof(UserMessage),4)), _result(0)
      {}
      ~ConfigAction() {}
    public:
      Transition* fire(Transition* tr);
      InDatagram* fire(InDatagram* in);
    private:
      ConfigCache& _cfg;
      const SrvL&  _server;
      Pds::Appliance& _app;
      GenericPool*         _occPool;
      unsigned             _result;
    };
          
    class BeginCalibCycleAction : public Action {
    public:
      BeginCalibCycleAction(ConfigCache& cfg, const SrvL& s, Pds::Appliance& app)
        : _cfg(cfg), _server(s), _app(app),
          _occPool(new GenericPool(sizeof(UserMessage),4)), _result(0)
      {}
    public:
      Transition* fire(Transition* tr);
      InDatagram* fire(InDatagram* in);
    private:
      ConfigCache&          _cfg;
      const SrvL&           _server;
      Pds::Appliance&       _app;
      GenericPool*          _occPool;
      unsigned              _result;
    };

    class UnconfigAction : public Action {
    public:
      UnconfigAction( ConfigCache& cfg, const SrvL& server ) : _cfg(cfg), _server( server ), _result(0) {}
      ~UnconfigAction() {}
    public:
      Transition* fire(Transition* tr);
      InDatagram* fire(InDatagram* in);
    private:
      ConfigCache& _cfg;
      const SrvL&  _server;
      unsigned     _result;
    };

    class EndCalibCycleAction : public Action {
    public:
      EndCalibCycleAction(ConfigCache& cfg, const SrvL& s) : _cfg(cfg), _server(s), _result(0) {};
    public:
      Transition* fire(Transition* tr);
      InDatagram* fire(InDatagram* in);
    private:
      ConfigCache& _cfg;
      const SrvL&  _server;
      unsigned     _result;
    };
  };
};

using namespace Pds::Epix10ka2m;
using Pds::InDatagram;
using Pds::Transition;

InDatagram* ConfigAction::fire(InDatagram* in) {
  printf("%s\n",__PRETTY_FUNCTION__);
  _cfg.record(in);

  for(SrvL::const_iterator it=_server.begin(); it!=_server.end(); it++)
    (*it)->recordExtraConfig(in);

  if( _result ) {
    printf( "*** Epix10ka2M::ConfigAction found configuration errors _result(0x%x)\n", _result );
    if (in->datagram().xtc.damage.value() == 0) {
      in->datagram().xtc.damage.increase(Damage::UserDefined);
      in->datagram().xtc.damage.userBits(_result);
    }
    char message[400];
    sprintf(message, "Epix10ka2M  on host %s failed to configure!\n", getenv("HOSTNAME"));
    UserMessage* umsg = new (_occPool) UserMessage;
    umsg->append(message);
    umsg->append(DetInfo::name(static_cast<const DetInfo&>(_server[0]->client())));
    umsg->append("\n");
    for(SrvL::const_iterator it=_server.begin(); it!=_server.end(); it++) {
      umsg->append((*it)->configurator()->pgp()->errorString());
      (*it)->configurator()->pgp()->clearErrorString();
    }
    printf("Epix10ka2M::ConfigAction %s", umsg->msg());
    _app.post(umsg);
  }
  return in;
}

Transition* BeginCalibCycleAction::fire(Transition* tr) {
  printf("%s\n",__PRETTY_FUNCTION__);
  if (_cfg.scanning() && _cfg.changed())
    _result = _cfg.configure(_server);
  if (_result)
    printf("\nEpix10ka2M::BeginCalibCycleAction::fire(tr) failed configuration\n");

  for(SrvL::const_iterator it=_server.begin(); it!=_server.end(); it++)
    (*it)->enable();
  printf("%s enabled\n",__PRETTY_FUNCTION__);
  return tr;
}

InDatagram* BeginCalibCycleAction::fire(InDatagram* in) {
  printf("%s\n",__PRETTY_FUNCTION__);
  if (_cfg.scanning() && _cfg.changed()) {
    _cfg.record(in);

    for(SrvL::const_iterator it=_server.begin(); it!=_server.end(); it++)
      (*it)->recordExtraConfig(in);
  }
  if( _result ) {
    printf( "*** Found configuration errors _result(0x%x)\n", _result );
    if (in->datagram().xtc.damage.value() == 0) {
      in->datagram().xtc.damage.increase(Damage::UserDefined);
      in->datagram().xtc.damage.userBits(_result);
    }
    char message[400];
    sprintf(message, "Epix10ka2M  on host %s failed to configure!\n", getenv("HOSTNAME"));
    UserMessage* umsg = new (_occPool) UserMessage;
    umsg->append(message);
    umsg->append(DetInfo::name(static_cast<const DetInfo&>(_server[0]->client())));
    umsg->append("\n");
    for(SrvL::const_iterator it=_server.begin(); it!=_server.end(); it++) {
      umsg->append((*it)->configurator()->pgp()->errorString());
      (*it)->configurator()->pgp()->clearErrorString();
    }
    printf("Epix10kaBeginCalibCycleAction %s\n", umsg->msg());
    _app.post(umsg);
  }
  return in;
}


Transition* UnconfigAction::fire(Transition* tr) {
  printf("%s\n",__PRETTY_FUNCTION__);
  _result = 0;
  for(SrvL::const_iterator it=_server.begin(); it!=_server.end(); it++)
    _result |= (*it)->unconfigure();
  return tr;
}

InDatagram* UnconfigAction::fire(InDatagram* in) {
  printf("%s\n",__PRETTY_FUNCTION__);
  if( _result ) {
    printf( "*** Found %d epix10ka2M Unconfig errors\n", _result );
    in->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
    in->datagram().xtc.damage.userBits(0xda);
  }
  return in;
}


Transition* EndCalibCycleAction::fire(Transition* tr) {
  printf("%s %p\n",__PRETTY_FUNCTION__,_cfg.current());
  _cfg.next();
  printf(" %p\n", _cfg.current());

  return tr;
}

InDatagram* EndCalibCycleAction::fire(InDatagram* in) {
  printf("%s\n",__PRETTY_FUNCTION__);
  if( _result ) {
    printf( "*** Epix10kaEndCalibCycleAction found unconfigure errors _result(0x%x)\n", _result );
    if (in->datagram().xtc.damage.value() == 0) {
      in->datagram().xtc.damage.increase(Damage::UserDefined);
      in->datagram().xtc.damage.userBits(_result);
    }
  }
  for(SrvL::const_iterator it=_server.begin(); it!=_server.end(); it++)
    (*it)->disable();
  return in;
}
#ifdef DBUG
static void _dump32(const void* p, unsigned nw)
{
  const uint32_t* u = reinterpret_cast<const uint32_t*>(p);
  for(unsigned i=0; i<nw; i++)
    printf("%08x%c", u[i], (i%8)==7 ? '\n':' ');
  if (nw%8) printf("\n");
}
#endif
InDatagram* L1Action::fire(InDatagram* in) {
  //  Create a new datagram from the pool
  Datagram&   dg = in->datagram();
#ifdef DBUG
  printf("L1Action: indg\n");
  _dump32(&dg, 32);
#endif
  if (!_pool.numberOfFreeObjects()) {
    dg.xtc.damage.increase(Damage::ContainsIncomplete);
    return in;
  }

  // Damage here is either missing quadrant or missing EVR
  // Missing EVR is common when master EVR rate differs from 
  // PGPEVR rate.  Drop all with damage.
  //  == Can't drop in WorkThreads == Mark non-iterable
  if (in->datagram().xtc.damage.value() != 0) {
    dg.xtc.damage.increase(Damage::IncompleteContribution);
    return in;
  }

  CDatagram* cdg = new (&_pool) CDatagram(dg);
  cdg->datagram().xtc.damage = in->datagram().xtc.damage;

  _cfg.reformat(dg,cdg->datagram());

#ifdef DBUG
  printf("L1Action: outdg\n");
  _dump32(&cdg->datagram(), 32);
#endif
  return cdg;
}

Transition* ConfigAction::fire(Transition* tr) {
  _result = 0;
  int i = _cfg.fetch(tr);
  printf("%s fetched %d: server size %zu\n",
         __PRETTY_FUNCTION__,i,_server.size());
  if (_cfg.scanning() == false) {
    _result = _cfg.configure(_server);
    if (_result)
      printf("\nEpix10ka2M::ConfigAction::fire(tr) failed configuration\n");
  }
  return tr;
}

Manager::Manager( const std::vector<Epix10ka2m::Server*>& server, unsigned nthreads) :
  _cfg(*new ConfigCache(server[0]->client())) 
{
   printf("Manager being initialized... " );

   Fsm& fsm = *new Fsm;

   if (nthreads) {
     std::vector<Appliance*> apps(4);
     apps[0] = &fsm;
     for(unsigned i=1; i<nthreads; i++) {
       Fsm* f = new Fsm;
       f->callback( TransitionId::L1Accept       , new L1Action             ( _cfg ) );
       apps[i] = f;
     }

     _app = new WorkThreads("wt2m",apps);
   }
   else {
     _app = &fsm;
   }

   fsm.callback( TransitionId::Map            , new AllocAction          ( _cfg, server ) );
   fsm.callback( TransitionId::Unmap          , new UnmapAction          ( ) );
   fsm.callback( TransitionId::Configure      , new ConfigAction         ( _cfg, server, *_app ) );
   fsm.callback( TransitionId::BeginCalibCycle, new BeginCalibCycleAction( _cfg, server, *_app ) );
   fsm.callback( TransitionId::EndCalibCycle  , new EndCalibCycleAction  ( _cfg, server ) );
   fsm.callback( TransitionId::L1Accept       , new L1Action             ( _cfg ) );
   fsm.callback( TransitionId::Unconfigure    , new UnconfigAction       ( _cfg, server ) );
}
