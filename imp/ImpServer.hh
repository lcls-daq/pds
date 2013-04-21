#ifndef IMPSERVER
#define IMPSERVER

#include "pds/utility/EbServer.hh"
#include "pds/utility/EbCountSrv.hh"
#include "pds/utility/EbEventKey.hh"
#include "pds/config/ImpConfigType.hh"
#include "pds/service/Task.hh"
//#include "pdsdata/imp/ElementHeader.hh"
#include "pds/imp/ImpConfigurator.hh"
#include "pds/imp/ImpDestination.hh"
#include "pds/imp/Processor.hh"
#include "pdsdata/xtc/Xtc.hh"
#include <fcntl.h>
#include <time.h>

namespace Pds
{
   class ImpServer;
   class Task;
}

class Pds::ImpServer
   : public EbServer,
     public EbCountSrv
{
 public:
   ImpServer( const Src& client, unsigned configMask=0 );
   virtual ~ImpServer() {}
    
   //  Eb interface
   void       dump ( int detail ) const {}
   bool       isValued( void ) const    { return true; }
   const Src& client( void ) const      { return _xtc.src; }

   //  EbSegment interface
   const Xtc& xtc( void ) const    { return _xtc; }
   unsigned   offset( void ) const;
   unsigned   length() const       { return _xtc.extent; }

   //  Eb-key interface
   EbServerDeclare;

   //  Server interface
   int pend( int flag = 0 ) { return -1; }
   int fetch( ZcpFragment& , int flags ) { return 0; }
   int fetch( char* payload, int flags );
   bool more() const;

   unsigned count() const;
   void setImp( int fd );

   unsigned configure(ImpConfigType*);
   unsigned unconfigure(void);

   unsigned payloadSize(void)   { return _payloadSize; }
   unsigned flushInputQueue(int);
   void     enable();
   void     disable();
   void     die();
   void     debug(unsigned d) { _debug = d; }
   unsigned debug() { return _debug; }
   void     offset(unsigned c) { _offset = c; }
   unsigned offset() { return _offset; }
   void     resetOffset() { _offset = 0; _count = 0xffffffff; }
   unsigned myCount() { return _count; }
   void     dumpFrontEnd();
   void     printHisto(bool);
   void     process(void);
   void     allocated();

 public:
   static ImpServer* instance() { return _instance; }
//   void                laneTest();

 private:
   static ImpServer*            _instance;
   static void instance(ImpServer* s) { _instance = s; }

 private:
   enum     {sizeOfHisto=1000, ElementsPerSegmentLevel=1};
   Xtc                            _xtc;
   Pds::Imp::ImpConfigurator* _cnfgrtr;
   unsigned                       _count;
   unsigned                       _payloadSize;
   unsigned                       _configMask;
   unsigned                       _configureResult;
   unsigned                       _debug;
   unsigned                       _offset;
   timespec                       _thisTime;
   timespec                       _lastTime;
   unsigned*                      _histo;
   Pds::Task*                     _task;
   unsigned                       _ioIndex;
   Pds::Imp::ImpDestination   _d;
   Pds::Pgp::Pgp*                 _pgp;
   unsigned                       _unconfiguredErrors;
   unsigned                       _compensateNoCountReset;
   bool                           _configured;
   bool                           _firstFetch;
   bool                           _getNewComp;
   bool                           _ignoreFetch;
};

#endif
