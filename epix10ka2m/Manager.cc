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
#include "pds/config/EpixConfigType.hh"
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
#include "pds/pgp/DataImportFrame.hh"
#include "pds/pgp/Pgp.hh"

//#define DBUG

typedef std::vector<Pds::Epix10ka2m::Server*> SrvL;

namespace Pds {
  class Allocation;
  class Transition;
  class InDatagram;

  namespace Epix10ka2m {
    class ConfigCache : public CfgCache {
    public:
      enum {StaticALlocationNumberOfConfigurationsForScanning=100};
      ConfigCache(const Src& src) :
        CfgCache(src, _epix10ka2MConfigType, StaticALlocationNumberOfConfigurationsForScanning * __size()) {}
    public:
      void printCurrent() {
        Epix10ka2MConfigType* cfg = (Epix10ka2MConfigType*)current();
        printf("%s current 0x%x\n", __PRETTY_FUNCTION__, (unsigned) (unsigned long)cfg);
      }
    private:
      int _size(void* tc) const { return __size(); }
      static int __size() { return Epix10ka2MConfigType::_sizeof(); }
    };

    class FrameBuilder : public Pds::XtcIterator {
    public:
      FrameBuilder(const Xtc* in, 
                   Xtc*       out, 
                   const Epix10ka2MConfigType&);
    public:
      int process(Xtc*);
    private:
      Epix10kaDataArray*        _payload;
      ndarray<const uint16_t,3> _array;
      ndarray<const uint16_t,3> _calib;
      ndarray<const uint32_t,3> _env;
      //      ndarray<const uint16_t,2> _temp;
    };

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
      L1Action(const ConfigCache& cfg, const SrvL& svr) :
        _cfg   (cfg),
        _server(svr),
        _pool  (5<<20, 16)
      { printf("L1Action pool\n"); _pool.dump(); }
      
      InDatagram* fire(InDatagram* in);

    private:
      const ConfigCache& _cfg;
      const SrvL& _server;
      GenericPool _pool;
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

FrameBuilder::FrameBuilder(const Xtc*                  in, 
                           Xtc*                        out, 
                           const Epix10ka2MConfigType& cfg) :
  Pds::XtcIterator(const_cast<Xtc*>(in)),
  _payload(new (out->next()) Epix10kaDataArray),
  _array  (_payload->frame            (cfg)),
  _calib  (_payload->calibrationRows  (cfg)),
  _env    (_payload->environmentalRows(cfg))
  //  _temp   (_payload->temperatures     (cfg))
{
#ifdef DBUG
  printf("%s: array(%d,%d,%d)  calib(%d,%d,%d)  env(%d,%d,%d)\n",
         __PRETTY_FUNCTION__,
         _array.shape()[0],
         _array.shape()[1],
         _array.shape()[2],
         _calib.shape()[0],
         _calib.shape()[1],
         _calib.shape()[2],
         _env.shape()[0],
         _env.shape()[1],
         _env.shape()[2]);
  iterate();
  unsigned sz = Epix10kaDataArray::_sizeof(cfg);
  printf("Epix10aDataArray::size = 0x%x\n",sz);
  printf("\tnelem %d  nrows %d  ncols %d  nasics %d\n",
         cfg.numberOfElements(),
         cfg.numberOfReadableRows(),
         cfg.numberOfColumns(),
         cfg.numberOfAsics());
  out->alloc(sz);
  printf("out->next %p\n",out->next());
#else
  iterate();
  unsigned sz = Epix10kaDataArray::_sizeof(cfg);
  out->alloc(sz);
#endif
}

int FrameBuilder::process(Xtc* xtc)
{
  if (xtc->contains.value() != _epix10kaDataType.value()) {
    iterate(xtc);
    return 1;
  }

  //  Set the frame number
  const Pds::Pgp::DataImportFrame* e = reinterpret_cast<Pds::Pgp::DataImportFrame*>(xtc->payload());
  *reinterpret_cast<uint32_t*>(_payload) = e->_frameNumber;

#ifdef DBUG
  printf("%s\n",__PRETTY_FUNCTION__);
  for(unsigned i=0; i<5; i++)
    printf(" %08x",reinterpret_cast<const uint32_t*>(xtc)[i]);
  printf("\n");
  for(unsigned i=0; i<8; i++)
    printf(" %08x",reinterpret_cast<const uint32_t*>(e)[i]);
  printf("\n");
#endif

  //  Each quad arrives on a separate lane
  //  A super row crosses 2 elements; each element contains 2x2 ASICs
  const unsigned lane0        = Pds::Pgp::Pgp::portOffset();
  const unsigned asicRows     = Pds::Epix::Config10ka::_numberOfRowsPerAsic;
  const unsigned elemRowSize  = Pds::Epix::Config10ka::_numberOfAsicsPerRow*Pds::Epix::Config10ka::_numberOfPixelsPerAsicRow*sizeof(uint16_t);

  // Copy this quadrant into the correct location within cxtc 
  unsigned quad = e->lane()-lane0;

  const uint8_t* u = reinterpret_cast<const uint8_t*>(e+1);
  
#define MMCPY(dst,src,sz) {                       \
    printf("memcpy %zx %p 0x%x\n",                \
           const_cast<uint8_t*>(dst)-             \
           const_cast<uint8_t*>(_payload),        \
           const_cast<const uint8_t*>(src)-u,sz);       \
    memcpy(dst,src,sz); }
#define MMCPY(dst,src,sz) { memcpy(dst,src,sz); }

  // Frame data
  for(unsigned i=0; i<asicRows; i++) { // 4 super rows at a time
    unsigned dnRow = asicRows+i;
    unsigned upRow = asicRows-i-1;
    MMCPY(const_cast<uint16_t*>(&_array(4*quad+2,dnRow,0)), u, elemRowSize); u += elemRowSize;
    MMCPY(const_cast<uint16_t*>(&_array(4*quad+3,dnRow,0)), u, elemRowSize); u += elemRowSize;
    MMCPY(const_cast<uint16_t*>(&_array(4*quad+2,upRow,0)), u, elemRowSize); u += elemRowSize;
    MMCPY(const_cast<uint16_t*>(&_array(4*quad+3,upRow,0)), u, elemRowSize); u += elemRowSize;
    MMCPY(const_cast<uint16_t*>(&_array(4*quad+0,dnRow,0)), u, elemRowSize); u += elemRowSize;
    MMCPY(const_cast<uint16_t*>(&_array(4*quad+1,dnRow,0)), u, elemRowSize); u += elemRowSize;
    MMCPY(const_cast<uint16_t*>(&_array(4*quad+0,upRow,0)), u, elemRowSize); u += elemRowSize;
    MMCPY(const_cast<uint16_t*>(&_array(4*quad+1,upRow,0)), u, elemRowSize); u += elemRowSize;
  }

  // Calibration rows
  const unsigned calibRows = _calib.shape()[1];
  for(unsigned i=0; i<calibRows; i++) {
    MMCPY(const_cast<uint16_t*>(&_calib(4*quad+2,i,0)), u, elemRowSize); u += elemRowSize;
    MMCPY(const_cast<uint16_t*>(&_calib(4*quad+3,i,0)), u, elemRowSize); u += elemRowSize;
    MMCPY(const_cast<uint16_t*>(&_calib(4*quad+0,i,0)), u, elemRowSize); u += elemRowSize;
    MMCPY(const_cast<uint16_t*>(&_calib(4*quad+1,i,0)), u, elemRowSize); u += elemRowSize;
  }

  // Environmental rows
  const unsigned envRows = _env.shape()[1];
  for(unsigned i=0; i<envRows; i++) {
    MMCPY(const_cast<uint32_t*>(&_env(4*quad+2,i,0)), u, elemRowSize); u += elemRowSize;
    MMCPY(const_cast<uint32_t*>(&_env(4*quad+3,i,0)), u, elemRowSize); u += elemRowSize;
    MMCPY(const_cast<uint32_t*>(&_env(4*quad+0,i,0)), u, elemRowSize); u += elemRowSize;
    MMCPY(const_cast<uint32_t*>(&_env(4*quad+1,i,0)), u, elemRowSize); u += elemRowSize;
  }

  // Temperatures
#if 0
  const unsigned tempSize = 4*sizeof(uint16_t);
  MMCPY(const_cast<uint16_t*>(&_temp(4*quad+2)), u, tempSize); u += tempSize;
  MMCPY(const_cast<uint16_t*>(&_temp(4*quad+3)), u, tempSize); u += tempSize;
  MMCPY(const_cast<uint16_t*>(&_temp(4*quad+0)), u, tempSize); u += tempSize;
  MMCPY(const_cast<uint16_t*>(&_temp(4*quad+1)), u, tempSize); u += tempSize;
#endif
  return 1;
}

InDatagram* ConfigAction::fire(InDatagram* in) {
  printf("%s\n",__PRETTY_FUNCTION__);
  _cfg.record(in);
  /*
  if (_server->samplerConfig()) {
    printf("Epix10kaConfigAction sending sampler config\n");
    in->insert(_server->xtcConfig(), _server->samplerConfig());
  } else {
    printf("Epix10kaConfigAction not sending sampler config\n");
  }
  if (_server->debug() & 0x10) _cfg.printCurrent();
  */
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
    umsg->append(DetInfo::name(static_cast<const DetInfo&>(_server[0]->xtc().src)));
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
  int result;
  if (_cfg.scanning() && _cfg.changed()) {
    const Epix10ka2MConfigType* c = reinterpret_cast<const Epix10ka2MConfigType*>(_cfg.current());
    for(unsigned q=0; q<_server.size(); q++) 
      if ((result = _server[q]->configure( c->evr(), c->quad(q), const_cast<Epix::Config10ka*>(&c->elemCfg(q*4)) ))) {
        printf("\nEpix10ka2M::BeginCalibCycleAction::fire(tr) failed config\n");
        _result |= result;
      };
  }
  for(SrvL::const_iterator it=_server.begin(); it!=_server.end(); it++)
    (*it)->enable();
  printf("%s enabled\n",__PRETTY_FUNCTION__);
  return tr;
}

InDatagram* BeginCalibCycleAction::fire(InDatagram* in) {
  printf("%s\n",__PRETTY_FUNCTION__);
  if (_cfg.scanning() && _cfg.changed()) {
    _cfg.record(in);
    /*
    if (_server->samplerConfig()) {
      printf("Epix10kaBeginCalibCycleAction sending sampler config\n");
      in->insert(_server->xtcConfig(), _server->samplerConfig());
    } else {
      printf("Epix10kaBeginCalibCycleAction not sending sampler config\n");
    }
    if (_server->debug() & 0x10) _cfg.printCurrent();
    */
  }
  if( _result ) {
    printf( "*** Epix10kaConfigAction found configuration errors _result(0x%x)\n", _result );
    if (in->datagram().xtc.damage.value() == 0) {
      in->datagram().xtc.damage.increase(Damage::UserDefined);
      in->datagram().xtc.damage.userBits(_result);
    }
    char message[400];
    sprintf(message, "Epix10ka2M  on host %s failed to configure!\n", getenv("HOSTNAME"));
    UserMessage* umsg = new (_occPool) UserMessage;
    umsg->append(message);
    umsg->append(DetInfo::name(static_cast<const DetInfo&>(_server[0]->xtc().src)));
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

static void _dump32(const void* p, unsigned nw)
{
  const uint32_t* u = reinterpret_cast<const uint32_t*>(p);
  for(unsigned i=0; i<nw; i++)
    printf("%08x%c", u[i], (i%8)==7 ? '\n':' ');
  if (nw%8) printf("\n");
}

InDatagram* L1Action::fire(InDatagram* in) {
  if (_server[0]->debug() & 8)
    printf("%s\n",__PRETTY_FUNCTION__);

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
  if (in->datagram().xtc.damage.value() != 0) {
    return 0;
  }

  CDatagram* cdg = new (&_pool) CDatagram(dg);
  cdg->datagram().xtc.damage = in->datagram().xtc.damage;

  const Xtc*      xtc = (const Xtc*)dg.xtc.payload();
  //
  //  Reformat the assembled datagram into a new pool buffer
  //
  Xtc* cxtc = new ((char*)cdg->xtc.next()) 
    Xtc(_epix10kaDataArray, xtc->src);
#ifdef DBUG
  printf("cdg %p  xtc.next %p  cxtx %p\n", 
         cdg, cdg->xtc.next(), cxtc);
#endif  
  FrameBuilder builder(&dg.xtc, cxtc, *reinterpret_cast<const Epix10ka2MConfigType*>(_cfg.current()));
  cdg->xtc.alloc(cxtc->extent);
#ifdef DBUG
  printf("L1Action: outdg\n");
  _dump32(&cdg->datagram(), 32);
#endif
  return cdg;
}

Transition* ConfigAction::fire(Transition* tr) {
  _result = 0;
  int i = _cfg.fetch(tr);
  printf("%s fetched %d: server size %u\n",
         __PRETTY_FUNCTION__,i,_server.size());
  if (_cfg.scanning() == false) {
    for(unsigned q=0; q<_server.size(); q++) {
      const Epix10ka2MConfigType* c = reinterpret_cast<const Epix10ka2MConfigType*>(_cfg.current());
      int result;
      if ((result = _server[q]->configure( c->evr(), c->quad(q), const_cast<Epix::Config10ka*>(&c->elemCfg(q*4)) ))) {
        printf("\nEpix10ka2M::ConfigAction::fire(tr) failed config\n");
        _result |= result;
      }
    }
    if (_result)
      printf("\nEpix10ka2M::ConfigAction::fire(tr) failed configuration\n");
  }
  return tr;
}

Manager::Manager( const std::vector<Epix10ka2m::Server*>& server) :
  _fsm(*new Fsm), _cfg(*new ConfigCache(server[0]->client())) 
{
   printf("Manager being initialized... " );

   _fsm.callback( TransitionId::Map            , new AllocAction          ( _cfg, server ) );
   _fsm.callback( TransitionId::Unmap          , new UnmapAction          ( ) );
   _fsm.callback( TransitionId::Configure      , new ConfigAction         ( _cfg, server, _fsm ) );
   _fsm.callback( TransitionId::BeginCalibCycle, new BeginCalibCycleAction( _cfg, server, _fsm ) );
   _fsm.callback( TransitionId::EndCalibCycle  , new EndCalibCycleAction  ( _cfg, server ) );
   _fsm.callback( TransitionId::L1Accept       , new L1Action             ( _cfg, server ) );
   _fsm.callback( TransitionId::Unconfigure    , new UnconfigAction       ( _cfg, server ) );
}
