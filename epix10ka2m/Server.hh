/*-------------------------------------------------------------------------------
-- Title      : 
-------------------------------------------------------------------------------
-- File       : Server.hh
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

#ifndef Pds_Epix10ka2m_Server_hh
#define Pds_Epix10ka2m_Server_hh

#include "pds/utility/EbServer.hh"
#include "pds/utility/EbEventKey.hh"
#include "pds/utility/BldSequenceSrv.hh"
#include "pds/utility/Transition.hh"
#include "pds/config/EpixConfigType.hh"
#include "pds/config/EpixSamplerConfigType.hh"
#include "pds/service/Task.hh"
#include "pds/epix10ka2m/Destination.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pds/service/GenericPool.hh"
#include "pds/evgr/EvrSyncCallback.hh"
#include "pds/evgr/EvrSyncRoutine.hh"
#include <fcntl.h>
#include <time.h>

namespace Pds {
  class Task;
  class Allocation;
  class Routine;
  class EvrSyncCallback;
  class InDatagram;

  namespace Epix10ka2m {
    class Configurator;
    class ServerSequence;
    class SyncSlave;

    class Server : public EbServer {
    public:
      virtual ~Server() {}
      virtual void     allocated  (const Allocate& a) = 0;
      virtual unsigned configure  (const Pds::Epix::PgpEvrConfig&,
                                   const Epix10kaQuadConfig&, 
                                   Pds::Epix::Config10ka*,
                                   bool forceConfig = false) = 0;
      virtual unsigned unconfigure(void) = 0;
      virtual void     enable     () = 0;
      virtual void     disable    () = 0;
      virtual unsigned fiducials  () const = 0;
      virtual bool     g3sync     () const = 0;
      virtual void     ignoreFetch(bool b) = 0;
      virtual Configurator* configurator() = 0;
      virtual void     dumpFrontEnd() = 0;
      virtual void     die         () = 0;
      virtual void     recordExtraConfig(InDatagram*) const = 0;
    public:
      static Server* instance();
      static void    instance(Server*);
      
      static  unsigned debug();
      static  void     debug(unsigned);

      static  unsigned resetOnEveryConfig();
      static  void     resetOnEveryConfig(unsigned);
    };

    class ServerSequence : public Server,
                           public BldSequenceSrv {
    public :
      ServerSequence(const Src& client, unsigned configMask=0 );
      virtual ~ServerSequence() {}
    
      //  Eb-key interface
      EbServerDeclare;
      unsigned fiducials() const;

      //  Eb interface
      void       dump ( int detail ) const {}
      bool       isValued( void ) const    { return true; }
      const Src& client( void ) const      { return _xtcTop.src; }
      
      //  EbSegment interface
      const Xtc& xtc( void ) const    { return _payload; }
      unsigned   length() const       { return _payload.extent; }

      //  Server interface
      int pend( int flag = 0 ) { return -1; }
      int fetch( char* payload, int flags );
      bool more() const;
      
      enum {DummySize = (1<<21)};
      
      void setFd( int fd, int fd2, unsigned lane );
      
      unsigned configure(const Pds::Epix::PgpEvrConfig&,
                         const Epix10kaQuadConfig&, 
                         Pds::Epix::Config10ka*,
                         bool forceConfig = false);
      unsigned unconfigure(void);
      
      unsigned payloadSize(void)   { return _payloadSize; }
      unsigned flushInputQueue(int, bool printFlag = true);
      void     enable();
      void     disable();
      bool     g3sync() const { return _g3sync; }
      void     g3sync(bool b) { _g3sync = b; }
      bool     ignoreFetch() { return _ignoreFetch; }
      void     ignoreFetch(bool b) { _ignoreFetch = b; }
      void     die();
      void     recordExtraConfig(InDatagram*) const;
      void     dumpFrontEnd();
      void     printHisto(bool);
      void     clearHisto();
      void     allocated(const Allocate& a) {_partition = a.allocation().partitionid(); }
      bool     resetOnEveryConfig() { return _resetOnEveryConfig; }
      void     resetOnEveryConfig(bool r) { _resetOnEveryConfig = r; }
      bool     scopeEnabled() { return _scopeEnabled; }
      Configurator* configurator() {return _cnfgrtr;}

    protected:
      enum     {sizeOfHisto=1000};
      Xtc                            _xtcTop;
      Xtc                            _xtcEpix;
      Xtc                            _xtcSamplr;
      Xtc                            _xtcConfig;
      Xtc                            _payload;
      Pds::EpixSampler::ConfigV1*     _samplerConfig;
      unsigned                       _fiducials;
      Configurator*                  _cnfgrtr;
      unsigned                       _payloadSize;
      unsigned                       _configureResult;
      timespec                       _thisTime;
      timespec                       _lastTime;
      unsigned*                      _histo;
      unsigned                       _ioIndex;
      Destination                    _d;
      GenericPool*                   _occPool;
      unsigned                       _unconfiguredErrors;
      float                          _timeSinceLastException;
      unsigned                       _fetchesSinceLastException;
      unsigned*                      _scopeBuffer;
      Pds::Task*                     _task;
      Task*                          _sync_task;
      unsigned                       _partition;
      SyncSlave*                     _syncSlave;
      unsigned                       _countBase;
      unsigned                       _neScopeCount;
      unsigned*                      _dummy;
      int                            _myfd;
      unsigned                       _lastOpCode;
      bool                           _firstconfig;
      bool                           _configured;
      bool                           _firstFetch;
      bool                           _g3sync;
      bool                           _ignoreFetch;
      bool                           _resetOnEveryConfig;
      bool                           _scopeEnabled;
      bool                           _scopeHasArrived;
    };
  };
};

#endif
