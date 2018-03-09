/*
 * Epix10kaServer.hh
 *
 *  Created on: 2017.10.27
 *      Author: jackp
 */

#ifndef EPIX10KASERVER
#define EPIX10KASERVER

#include "pds/utility/EbServer.hh"
#include "pds/utility/EbEventKey.hh"
#include "pds/config/EpixConfigType.hh"
#include "pds/config/EpixSamplerConfigType.hh"
#include "pds/service/Task.hh"
#include "pds/epix10ka/Epix10kaManager.hh"
#include "pds/epix10ka/Epix10kaConfigurator.hh"
#include "pds/epix10ka/Epix10kaDestination.hh"
#include "pds/utility/Occurrence.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pds/service/GenericPool.hh"
#include "pds/evgr/EvrSyncCallback.hh"
#include "pds/evgr/EvrSyncRoutine.hh"
#include <fcntl.h>
#include <time.h>

namespace Pds
{
  class Epix10kaServer;
  class Epix10kaSyncSlave;
  class Task;
  class Allocation;
  class Routine;
  class EvrSyncCallback;
  class Epix10kaServerSequence;
  class Epix10kaServerCount;
  class EbCountSrv;
  class BldSequenceSrv;
}

class Pds::Epix10kaServer
   : public EbServer
{
 public:
   Epix10kaServer( const Src& client, unsigned configMask=0 );
   virtual ~Epix10kaServer() {}
    
   //  Eb interface
   void       dump ( int detail ) const {}
   bool       isValued( void ) const    { return true; }
   const Src& client( void ) const      { return _xtcTop.src; }

   //  EbSegment interface
   const Xtc& xtc( void ) const    { return _xtcTop; }
   unsigned   offset( void ) const;
   unsigned   length() const       { return _xtcTop.extent; }

   //  Server interface
   int pend( int flag = 0 ) { return -1; }
   int fetch( char* payload, int flags );
   bool more() const;

   enum {DummySize = (1<<19)};

   void setEpix10ka( int fd );

   unsigned configure(Epix10kaConfigType*, bool forceConfig = false);
   unsigned unconfigure(void);

   unsigned payloadSize(void)   { return _payloadSize; }
   unsigned numberOfElements(void) { return _elements; }
   unsigned flushInputQueue(int, bool printFlag = true);
   void     enable();
   void     disable();
   bool     g3sync() { return _g3sync; }
   void     g3sync(bool b) { _g3sync = b; }
   bool     ignoreFetch() { return _ignoreFetch; }
   void     ignoreFetch(bool b) { _ignoreFetch = b; }
   void     die();
   void     debug(unsigned d) { _debug = d; }
   unsigned debug() { return _debug; }
   void     offset(unsigned c) { _offset = c; }
   unsigned offset() { return _offset; }
   void     resetOffset() { _offset = 0; _count = 0xffffffff; }
   unsigned myCount() { return _count; }
   void     dumpFrontEnd();
   void     printHisto(bool);
   void     clearHisto();
   void     manager(Epix10kaManager* m) { _mgr = m; }
   Epix10kaManager* manager() { return _mgr; }
   void     process(char*);
   void     allocated(const Allocate& a) {_partition = a.allocation().partitionid(); }
   bool     resetOnEveryConfig() { return _resetOnEveryConfig; }
   void     runTimeConfigName(char*);
   void     resetOnEveryConfig(bool r) { _resetOnEveryConfig = r; }
   void     fiberTriggering(bool b) { _fiberTriggering = b; }
   bool     fiberTriggering() { return _fiberTriggering; }
   bool     scopeEnabled() { return _scopeEnabled; }
   EpixSampler::ConfigV1*  samplerConfig() { return _samplerConfig; }
   const Xtc&      xtcConfig() { return _xtcConfig; }
   void     maintainLostRunTrigger(bool b) { _maintainLostRunTrigger = b; }
   Pds::Epix10ka::Epix10kaConfigurator* configurator() {return _cnfgrtr;}

 public:
   static Epix10kaServer* instance() { return _instance; }

 private:
   static Epix10kaServer*            _instance;
   static void instance(Epix10kaServer* s) { _instance = s; }

 protected:
   enum     {sizeOfHisto=1000, ElementsPerSegmentLevel=1};
   Xtc                            _xtcTop;
   Xtc                            _xtcEpix;
   Xtc                            _xtcSamplr;
   Xtc                            _xtcConfig;
   Pds::EpixSampler::ConfigV1*     _samplerConfig;
   unsigned                       _count;
   unsigned                       _fiducials;
   Pds::Epix10ka::Epix10kaConfigurator* _cnfgrtr;
   unsigned                       _elements;
   unsigned                       _elementsThisCount;
   unsigned                       _payloadSize;
   unsigned                       _configureResult;
   unsigned                       _debug;
   unsigned                       _offset;
   timespec                       _thisTime;
   timespec                       _lastTime;
   unsigned*                      _histo;
   unsigned                       _ioIndex;
   Pds::Epix10ka::Epix10kaDestination     _d;
   char                           _runTimeConfigName[256];
   Epix10kaManager*               _mgr;
   GenericPool*                   _occPool;
   unsigned                       _unconfiguredErrors;
   float                          _timeSinceLastException;
   unsigned                       _fetchesSinceLastException;
   char*                          _processorBuffer;
   unsigned*                      _scopeBuffer;
   Pds::Task*                     _task;
   Task*						              _sync_task;
   unsigned                       _partition;
   Epix10kaSyncSlave*             _syncSlave;
   unsigned                       _countBase;
   unsigned                       _neScopeCount;
   unsigned*                      _dummy;
   int                            _myfd;
   unsigned                       _lastOpCode;
   bool                           _firstconfig;
   bool                           _configured;
   bool                           _firstFetch;
   bool                           _g3sync;
   bool                           _fiberTriggering;
   bool                           _ignoreFetch;
   bool                           _resetOnEveryConfig;
   bool                           _scopeEnabled;
   bool                           _scopeHasArrived;
   bool                           _maintainLostRunTrigger;
};

#include "pds/utility/EbCountSrv.hh"
class Pds::Epix10kaServerCount
    :  public Epix10kaServer,
       public EbCountSrv {
         public:
    Epix10kaServerCount(const Src& client, unsigned configMask=0 )
         : Pds::Epix10kaServer(client, configMask) {}
    ~Epix10kaServerCount() {}
         public:
    //  Eb-key interface
    EbServerDeclare;
    unsigned count() const;
};

#include "pds/utility/BldSequenceSrv.hh"
class Pds::Epix10kaServerSequence
    :  public Epix10kaServer,
       public BldSequenceSrv {
         public :
    Epix10kaServerSequence(const Src& client, unsigned configMask=0 )
         : Pds::Epix10kaServer(client, configMask) {}
    ~Epix10kaServerSequence() {}
         public:
    //  Eb-key interface
    EbServerDeclare;
    unsigned fiducials() const;
};

#endif
