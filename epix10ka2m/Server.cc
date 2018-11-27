/*-------------------------------------------------------------------------------
-- Title      : 
-------------------------------------------------------------------------------
-- File       : Server.cc
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

#include "pds/epix10ka2m/Server.hh"
#include "pds/epix10ka2m/Configurator.hh"
#include "pds/epix10ka/Epix10kaDestination.hh"
#include "pds/xtc/CDatagram.hh"
#include "pdsdata/psddl/epix.ddl.h"
#include "pds/config/EpixConfigType.hh"
#include "pds/config/EpixSamplerConfigType.hh"
#include "pds/config/EpixSamplerDataType.hh"
#include "pds/config/EpixDataType.hh"
#include "pds/pgp/Pgp.hh"
#include "pds/pgp/DataImportFrame.hh"
#include "pds/pgp/RegisterSlaveExportFrame.hh"
#include "pds/evgr/EvrSyncCallback.hh"
#include "pds/evgr/EvrSyncRoutine.hh"
#include "pds/service/Routine.hh"
#include "pds/service/Task.hh"
#include "pds/service/TaskObject.hh"
#include "pds/xtc/XtcType.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include <PgpDriver.h>
#include <unistd.h>
#include <sys/uio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <omp.h>

//#define SHORT_PAYLOAD

using namespace Pds;

static Epix10ka2m::Server* _instance = 0;
Epix10ka2m::Server* Epix10ka2m::Server::instance() { return _instance; }
void                Epix10ka2m::Server::instance(Epix10ka2m::Server* s) { _instance = s; }

static unsigned _debug = 0;
unsigned Epix10ka2m::Server::debug() { return _debug; }
void     Epix10ka2m::Server::debug(unsigned d) { _debug=d; }

static unsigned _resetOnEveryConfig = 0;
unsigned Epix10ka2m::Server::resetOnEveryConfig() { return _resetOnEveryConfig; }
void     Epix10ka2m::Server::resetOnEveryConfig(unsigned d) { _resetOnEveryConfig=d; }

class Task;
class TaskObject;
class DetInfo;

long long int timeDiff(timespec* end, timespec* start) {
  long long int diff;
  diff =  (end->tv_sec - start->tv_sec);
  if (diff) diff *= 1000000000LL;
  diff += end->tv_nsec;
  diff -= start->tv_nsec;
  return diff;
}

namespace Pds {
  namespace Epix10ka2m {
    class SyncSlave : public EvrSyncCallback {
    public:
      SyncSlave(unsigned        partition,
                Task*           task,
                Server*         srvr);
      virtual ~SyncSlave() {
        delete _routine;
      };
    public:
      void initialize(unsigned, bool);
    private:
      Task&           _task;
      Routine*        _routine;
      Server*         _srvr;
      bool            _printed;
    };
  };
};

Epix10ka2m::SyncSlave::SyncSlave(unsigned partition,
                                 Task*    task,
                                 Server*  srvr) :
  _task(*task),
  _routine(new EvrSyncRoutine(partition,*this)),
  _srvr(srvr),
  _printed(false)
{
  _task.call(_routine);
}

void Epix10ka2m::SyncSlave::initialize(unsigned target, bool enable) {
  if (_srvr->g3sync() == true) {
    _srvr->configurator()->fiducialTarget(target);
    _srvr->configurator()->evrLaneEnable(enable);
    unsigned fid = _srvr->configurator()->getCurrentFiducial();
    printf("Epix10ka2m::SyncSlave target 0x%x, pgpcard fiducial 0x%x after %s\n",
           target, fid, enable ? "enabling" : "disabling");
    //if fiducial less than target or check for wrap
    if ((fid < target) || ((fid > (0x1ffff-32-720-7)) && (target < (720+7)))) {
    } else {
      printf("Fiducial was not before target so retrying four later than now ...\n");
      _srvr->configurator()->fiducialTarget(fid+4);
    }
    _srvr->ignoreFetch(false);
  } else {
    if (!_printed) {
      printf("g3sync was not true\n");
      _printed = true;
    }
  }
}

Epix10ka2m::ServerSequence::ServerSequence( const Src& client, unsigned configMask ) :
  _xtcTop(TypeId(TypeId::Id_Xtc,1), client),
  _xtcEpix( _epix10kaDataType, client ),
  _xtcSamplr(_epixSamplerDataType, client),
  _xtcConfig(TypeId(TypeId::Id_EpixSamplerConfig,2), client),
  _samplerConfig(0),
  _quad     (-1),
  _fiducials(0),
  _cnfgrtr(0),
  _payloadSize(0),
  _configureResult(0),
  _unconfiguredErrors(0),
  _timeSinceLastException(0),
  _fetchesSinceLastException(0),
  _scopeBuffer(0),
  _task      (new Pds::Task(Pds::TaskObject("EPIX10kaprocessor"))),
  _sync_task (new Pds::Task(Pds::TaskObject("Epix10kaSlaveSync"))),
  _partition(0),
  _syncSlave(0),
  _countBase(0),
  _neScopeCount(0),
  _dummy(0),
  _lastOpCode(0),
  _firstconfig(1),
  _configured(false),
  _firstFetch(true),
  _g3sync(false),
  _ignoreFetch(true),
  _scopeEnabled(false),
  _scopeHasArrived(false)
{
  _histo = (unsigned*)calloc(sizeOfHisto, sizeof(unsigned));
  instance(this);
  printf("Epix10ka2m::ServerSequence() payload(%u)\n", _payloadSize);
  _dummy = (unsigned*)malloc(DummySize);
}

void  Epix10ka2m::ServerSequence::setFd( int f, int f2, unsigned lane, unsigned quad ) {
  _myfd = f;
  fd(f);
  _cnfgrtr = new Epix10ka2m::Configurator(f2, lane, quad, _debug);
  _quad = quad;
}

unsigned Epix10ka2m::ServerSequence::configure(const Epix::PgpEvrConfig&     evr,
                                               const Epix10kaQuadConfig&     quad,
                                               Epix::Config10ka*             elems,
                                               bool                          forceConfig) {
  unsigned firstConfig = _resetOnEveryConfig || forceConfig;
  if (_firstconfig == true) {
    _firstconfig = false;
    firstConfig = 1;
    printf("Epix10kaServer::configure making new configurator %p, firstConfig %u\n", _cnfgrtr, firstConfig);
  }
  _xtcTop.extent = sizeof(Xtc);
  _xtcSamplr.extent = 0;
  unsigned c = flushInputQueue(fd());

  if ((_configureResult = _cnfgrtr->configure(evr, quad, elems, firstConfig))) {
    printf("Epix10kaServer::configure failed 0x%x, first %u\n", _configureResult, firstConfig);
  } else {
    // header + payload + trailer
    //    _payloadSize = 4*Pds::Epix::ElementV3::_sizeof(elems[0])-15*(32+4);
    _payloadSize = 1095232;  // need to understand this
    _xtcEpix.extent = _payloadSize + sizeof(Xtc);
    _xtcTop.extent += _xtcEpix.extent;

    if (_samplerConfig) {
      delete(_samplerConfig);
      _samplerConfig = 0;
    }
    _xtcSamplr.extent = 0;
    if (_scopeBuffer) {
      free(_scopeBuffer);
      _scopeBuffer = 0;
    }

    if ((_scopeEnabled = quad.scopeEnable())) {
      _samplerConfig = new EpixSamplerConfigType(
          1,  // version
          0, // config->epixRunTrigDelay(),
          0, // config->epixRunTrigDelay() + 20,
          0, // config->dacSetting(),
          quad.asicRoClkHalfT(),
          0, // config->adcPipelineDelay0(),
          elems[0].carrierId0(),
          elems[0].carrierId1(),
          0, // config->analogCardId0(),
          0, // config->analogCardId1(),
          2,  // number of Channels
          quad.scopeTraceLength(),
          quad.baseClockFrequency(),
          0  // testPatterEnable
      );
      _xtcSamplr.extent = EpixSamplerDataType::_sizeof(*_samplerConfig) + sizeof(Xtc);
      //      _xtcTop.extent += _xtcSamplr.extent;
      _xtcConfig.extent = sizeof( EpixSamplerConfigType) + sizeof(Xtc);
      _scopeBuffer = (unsigned*)calloc(_xtcSamplr.sizeofPayload(), 1);
    }

    if  (_syncSlave == 0) {
      _syncSlave = new Pds::Epix10ka2m::SyncSlave(_partition, _sync_task, this);
    }
    _g3sync = true;

    printf("Epix10kaServer::configure _payloadSize(%u) _xtcTop.extent(%u) _xtcEpix.extent(%u) firstConfig(%u)\n",
        _payloadSize, _xtcTop.extent, _xtcEpix.extent, firstConfig);

    _firstFetch = true;
    _countBase = 0;
  }
  _configured = _configureResult == 0;
  c += flushInputQueue(fd());
  if (c>0) {
    printf("Epix10kaServer::configure flushed %d buffers from input queue\n", c);
  }
  clearHisto();

  return _configureResult;
}

void Epix10ka2m::ServerSequence::die() {
    printf("Epix10kaServer::die has been called !!!!!!!\n");
}

void Epix10ka2m::ServerSequence::dumpFrontEnd() {
  if (_cnfgrtr) _cnfgrtr->dumpFrontEnd();
  else printf("Epix10kaServer::dumpFrontEnd() found nil configurator\n");
}

void Epix10ka2m::ServerSequence::enable() {
  if (usleep(10000)<0) perror("Epix10ka2m::ServerSequence::enable ulseep failed\n");
  //  _cnfgrtr->enableExternalTrigger(true);
  flushInputQueue(fd());
  _firstFetch = true;
  _countBase = 0;
  _ignoreFetch = _g3sync;
  if (_debug & 0x20) printf("Epix10ka2m::ServerSequence::enable\n");
}

void Epix10ka2m::ServerSequence::disable() {
  _ignoreFetch = true;
  if (_cnfgrtr) {
    //    _cnfgrtr->enableExternalTrigger(false);
    flushInputQueue(fd());
    if (usleep(10000)<0) perror("Epix10ka2m::ServerSequence::disable ulseep 1 failed\n");
    if (_debug & 0x20) printf("Epix10ka2m::ServerSequence::disable\n");
    //    dumpFrontEnd();
    printHisto(true);
  } else {
    printf("Epix10ka2m::ServerSequence::disable() found nil configurator, so did not disable\n");
  }
}

unsigned Epix10ka2m::ServerSequence::unconfigure(void) {
  unsigned c = flushInputQueue(fd());
  if (c) printf("Epix10ka2m::ServerSequence::unconfigure flushed %u event%s\n", c, c>1 ? "s" : "");
  _cnfgrtr->unconfigure();
  return 0;
}

int Epix10ka2m::ServerSequence::fetch( char* payload, int flags ) {
   int ret = 0;
   DmaReadData     pgpCardRx;
   unsigned        offset = 0;
   enum {Ignore=-1};

   if (_ignoreFetch) {
     flushInputQueue(fd(), false);
     return Ignore;
   }

   if (_configured == false)  {
     flushInputQueue(fd(), false);
     return Ignore;
   }

   _xtcTop .damage = 0;
   _xtcEpix.damage = 0;

   //  Insert sampler, if present
   memcpy(payload+offset, &_xtcTop , sizeof(Xtc)); offset += sizeof(Xtc);
   memcpy(payload+offset, &_xtcEpix, sizeof(Xtc)); offset += sizeof(Xtc);

   if (_debug & 3) printf("Epix10ka2m::ServerSequence::fetch called\n");

   pgpCardRx.data   = uintptr_t(payload+offset);
   pgpCardRx.dest   = 0;
   pgpCardRx.flags  = 0;
   pgpCardRx.index  = 0;
   pgpCardRx.error  = 0;
   pgpCardRx.size = _payloadSize;
   pgpCardRx.is32   = sizeof(&pgpCardRx) == 4;

   if ((ret = read(fd(), &pgpCardRx, sizeof(DmaReadData))) < 0) {
     if (errno == ERESTART) {
       disable();
       _ignoreFetch = true;
       printf("Epix10ka2m::ServerSequence::fetch exiting because of ERESTART\n");
       exit(-ERESTART);
     }
     perror ("Epix10ka2m::ServerSequence::fetch pgpCard read error");
     ret =  Ignore;
   };

   unsigned damageMask = 0;
   if (pgpCardRx.error) {
     if (pgpCardRx.error&DMA_ERR_FIFO)  damageMask |= 1;
     if (pgpCardRx.error&DMA_ERR_LEN)   damageMask |= 2;
     if (pgpCardRx.error&DMA_ERR_MAX)   damageMask |= 4;
     if (pgpCardRx.error&DMA_ERR_BUS)   damageMask |= 8;
   }

   if (pgpGetVc(pgpCardRx.dest) == Epix10ka::Epix10kaDestination::Oscilloscope) {
     
     if (damageMask) {
       _xtcSamplr.damage.increase(Damage::UserDefined);
       _xtcSamplr.damage.userBits(damageMask | 0xe0);
     } else {
       _xtcSamplr.damage = 0;
     }
     if (scopeEnabled())  {
       if (_scopeBuffer) {
         memcpy(_scopeBuffer, (char*)pgpCardRx.data, _xtcSamplr.sizeofPayload());
         _scopeHasArrived = true;
       }
     } else {
       printf("Epix10ka2m::ServerSequence::fetch ignoring scope buffer %u when not enabled!\n", ++_neScopeCount);
     }
     return Ignore;
   }

   Pds::Pgp::DataImportFrame* data = reinterpret_cast<Pds::Pgp::DataImportFrame*>(payload+offset);

   //   data->fixup(); //fix pgp header for v3 to v2
   data->first.lane = _quad;

   if (pgpGetVc(pgpCardRx.dest) == Epix10ka::Epix10kaDestination::Data) {

     if ((ret > 0) && (ret < (int)_payloadSize)) {
       printf("Epix10ka2m::ServerSequence::fetch() returning Ignore, ret was %d(%u)\n", ret, _payloadSize);

       // uint32_t* u = (uint32_t*)data;
       // printf("\tDataHeader: "); for (int i=0; i<16; i++) printf("0x%08x ", u[i]); printf("\n");
       ret = Ignore;
     }

     if (ret > (int) _payloadSize) printf("Epix10ka2m::ServerSequence::fetch pgp read returned too much _payloadSize(%u) ret(%d)\n", _payloadSize, ret);

     if (damageMask) {
       damageMask |= 0xe0;
       _xtcEpix.damage.increase(Damage::UserDefined);
       _xtcEpix.damage.userBits(damageMask);
       printf("Epix10ka2m::ServerSequence::fetch setting user damage 0x%x", damageMask);
       if (pgpCardRx.error&DMA_ERR_LEN) printf(", rxSize(%u), payloadSize(%u) ret(%d) offset(%u) (bytes)",
           (unsigned)pgpCardRx.size, _payloadSize, ret, offset);
       printf("\n");
     } else {
       _fiducials = data->fiducials();    // for fiber triggering
       if ((_debug & 5) || ret < 0) {
         printf("\telementId(%u) frameType(0x%x) acqcount(0x%x) lane(%u) vc(%u) quad(%u)\n",
                data->elementId(), data->_frameType, data->acqCount(),
                pgpGetLane(pgpCardRx.dest), pgpGetVc(pgpCardRx.dest), _quad);
         uint32_t* u = (uint32_t*)data;
         printf("\tDataHeader: "); for (int i=0; i<16; i++) printf("0x%x ", u[i]); printf("\n");
       }
     }
     if (_firstFetch) {
       _firstFetch = false;
       clock_gettime(CLOCK_REALTIME, &_lastTime);
     } else {
       clock_gettime(CLOCK_REALTIME, &_thisTime);
       long long int diff = timeDiff(&_thisTime, &_lastTime);
       if (diff > 0) {
         unsigned peak = 0;
         unsigned max = 0;
         diff += 500000;
         diff /= 1000000;
         _timeSinceLastException += (float)(diff & 0xffffffff);
         _fetchesSinceLastException += 1;
         if (diff > sizeOfHisto-1) diff = sizeOfHisto-1;
         _histo[diff] += 1;
         for (unsigned i=0; i<sizeOfHisto; i++) {
           if (_histo[i]) {
             if (_histo[i] > max) {
               max = _histo[i];
               peak = i;
             }
           }
         }
         if (max > 100) {
           if ( (diff >= (peak + 2)) || (diff <= (peak - 2)) ) {
             _timeSinceLastException /= 1000000.0;
//             printf("Epix10ka2m::ServerSequence::fetch exceptional period %3lld, not %3u, frame %5u, frames since last %5u, ms since last %7.3f, ms/f %6.3f\n",
//             diff, peak, _count, _fetchesSinceLastException, _timeSinceLastException, (1.0*_timeSinceLastException)/_fetchesSinceLastException);
//             printf("Epix10ka2m::ServerSequence::fetch exceptional period %3lld, not %3u, frames since last %5u\n",
//                 diff, peak, _fetchesSinceLastException);
             _timeSinceLastException = 0;
             _fetchesSinceLastException = 0;
           }
         }
       } else {
         printf("Epix10ka2m::ServerSequence::fetch Clock backtrack %f ms\n", diff / 1000000.0);
       }
//       if (g3sync() && (data->opCode() != (_lastOpCode + 1)%256)) {
//         printf("Epix10ka2m::ServerSequence::fetch opCode mismatch last(%u) this(%u) on frame(0x%x)\n",
//             _lastOpCode, data->opCode(), _count);
//       }
       memcpy(&_lastTime, &_thisTime, sizeof(timespec));
     }
     _lastOpCode = data->opCode();

     offset += ret;

     if (_scopeHasArrived) {
       reinterpret_cast<Xtc*>(payload)->extent += _xtcSamplr.extent;
       memcpy(payload + offset, &_xtcSamplr, sizeof(Xtc));
       offset += sizeof(Xtc);
       memcpy(payload + offset, _scopeBuffer, _xtcSamplr.sizeofPayload());
       offset += _xtcSamplr.sizeofPayload();
       _scopeHasArrived = false;

       if (_debug & 3) {
         printf("Epix10ka2m::ServerSequence::fetch attached oscilloscope size %u\n", _xtcSamplr.sizeofPayload());
         Xtc* x = reinterpret_cast<Xtc*>(payload);
         printf(" xtcTop   @ %p  ctns [0x%x]  extent [0x%x]\n", x, x->contains.value(), x->extent); x++;
         printf("  xtcEpix @ %p  ctns [0x%x]  extent [0x%x]\n", x, x->contains.value(), x->extent); x=x->next();
         printf("  xtcSmpl @ %p  ctns [0x%x]  extent [0x%x]\n", x, x->contains.value(), x->extent);
       }

     }
   } else {  // wrong vc for us
     return Ignore;
   }
   if (ret > 0) {
#ifdef SHORT_PAYLOAD
     ret = (new (new(payload) Xtc(_xtcTop)) Xtc(_xtcType, _xtcEpix.src))->extent;
#else
     ret = offset;
#endif
   }
   (_payload = *reinterpret_cast<const Xtc*>(payload)).extent = ret;
   if (_debug & 5) printf(" returned %d\n", ret);
   return ret;
}

bool Epix10ka2m::ServerSequence::more() const {
  bool ret = false;
  if (_debug & 2) printf("Epix10ka2m::ServerSequence::more(%s)\n", ret ? "true" : "false");
  return ret;
}

unsigned Epix10ka2m::ServerSequence::fiducials() const {
  if (_debug & 2) printf( "Epix10ka2m::ServerSequence::fiducials(%u)\n", _fiducials);
  return _fiducials;
}

unsigned Epix10ka2m::ServerSequence::flushInputQueue(int f, bool flag) {
  fd_set          fds;
  struct timeval  timeout;
  timeout.tv_sec  = 0;
  timeout.tv_usec = 2500;
  int ret;
  unsigned count = 0;
  unsigned scopeCount = 0;
  DmaReadData       pgpCardRx;
  pgpCardRx.is32   = sizeof(&pgpCardRx) == 4;
  pgpCardRx.size   = DummySize;
  pgpCardRx.data   = (uint64_t)_dummy;
  pgpCardRx.dest   = 0;
  pgpCardRx.flags  = 0;
  pgpCardRx.index  = 0;
  pgpCardRx.error  = 0;
  do {
    FD_ZERO(&fds);
    FD_SET(f,&fds);
    ret = select( f+1, &fds, NULL, NULL, &timeout);
    if (ret>0) {
      ::read(f, &pgpCardRx, sizeof(DmaReadData));
      if (pgpGetVc(pgpCardRx.dest) == Epix10ka::Epix10kaDestination::Oscilloscope) {
        scopeCount += 1;
      }
      count += 1;
    }
  } while (ret > 0);
  if (count && flag) {
    printf("Epix10ka2m::ServerSequence::flushInputQueue flushed %u scope buffers and %u other buffers\n", scopeCount, count-scopeCount);
  }
  return count;
}

void Epix10ka2m::ServerSequence::clearHisto() {
  for (unsigned i=0; i<sizeOfHisto; i++) {
    _histo[i] = 0;
  }
}

void Epix10ka2m::ServerSequence::printHisto(bool c) {
  unsigned histoSum = 0;
  printf("Epix10ka2m::ServerSequence event fetch periods\n");
  for (unsigned i=0; i<sizeOfHisto; i++) {
    if (_histo[i]) {
      printf("\t%3u ms   %8u\n", i, _histo[i]);
      histoSum += _histo[i];
      if (c) _histo[i] = 0;
    }
  }
  printf("\tHisto Sum was %u\n", histoSum);
  histoSum = 0;
}

void Epix10ka2m::ServerSequence::recordExtraConfig(InDatagram* in) const
{
  if (_samplerConfig)
    in->insert(_xtcConfig, _samplerConfig);
}
