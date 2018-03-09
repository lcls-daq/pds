/*
 * Epix10kaServer.cc
 *
 *  Created on: 2017.10.27
 *      Author: jackp
 */

#include "pds/epix10ka/Epix10kaServer.hh"
#include "pds/epix10ka/Epix10kaConfigurator.hh"
//#include "pds/epix10ka/Epix10kaDestination.hh"
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
#include "pds/epix10ka/Epix10kaDestination.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include <PgpDriver.h>
#include <unistd.h>
#include <sys/uio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <omp.h>

using namespace Pds;
//using namespace Pds::Epix10ka;

Epix10kaServer* Epix10kaServer::_instance = 0;

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

class Pds::Epix10kaSyncSlave : public EvrSyncCallback {
  public:
    Epix10kaSyncSlave(
		 unsigned        partition,
		 Pds::Task*           task,
		 Epix10kaServer* srvr);
    virtual ~Epix10kaSyncSlave() {
      delete _routine;
    };
  public:
    void initialize(unsigned, bool);
  private:
    Pds::Task&      _task;
    Routine*        _routine;
    Epix10kaServer* _srvr;
    bool            _printed;
};

Pds::Epix10kaSyncSlave::Epix10kaSyncSlave(
		unsigned partition,
		Pds::Task*    task,
		Epix10kaServer* srvr) :
				_task(*task),
				_routine(new EvrSyncRoutine(partition,*this)),
				_srvr(srvr) {
	printf("%s\n", "Epix10kaSyncSlave constructor");
	_printed = false;
	_task.call(_routine);
}

void Epix10kaSyncSlave::initialize(unsigned target, bool enable) {
  if (_srvr->g3sync() == true) {
    _srvr->configurator()->fiducialTarget(target);
    _srvr->configurator()->evrLaneEnable(enable);
    unsigned fid = _srvr->configurator()->getCurrentFiducial();
    printf("Epix10kaSyncSlave target 0x%x, pgpcard fiducial 0x%x after %s\n",
        target, fid, enable ? "enabling" : "disabling");
    //if fiducial less than target or check for wrap
    if ((fid < target) || ((fid > (0x1ffff-32-720-7)) && (target < (720+7)))) {
//      if (target-fid > 4) usleep((target-fid-2)*2777);
//      while (fid < (target + 2) && (_srvr->configurator()->getLatestLaneStatus() != enable)) {
//        fid = _srvr->configurator()->getCurrentFiducial();
//        if (fid == target) {
//          printf("\tthen saw 0x%x\n", fid);
//        }
//        usleep(80);
//      }
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

Epix10kaServer::Epix10kaServer( const Pds::Src& client, unsigned configMask )
   : _xtcTop(TypeId(TypeId::Id_Xtc,1), client),
     _xtcEpix( _epix10kaDataType, client ),
     _xtcSamplr(_epixSamplerDataType, client),
     _xtcConfig(TypeId(TypeId::Id_EpixSamplerConfig,2), client),
     _samplerConfig(0),
     _count(0),
     _fiducials(0),
     _cnfgrtr(0),
     _elements(ElementsPerSegmentLevel),
     _payloadSize(0),
     _configureResult(0),
     _debug(0),
     _offset(0),
     _unconfiguredErrors(0),
     _timeSinceLastException(0),
     _fetchesSinceLastException(0),
     _processorBuffer(0),
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
	   _fiberTriggering(false),
     _ignoreFetch(true),
     _resetOnEveryConfig(false),
     _scopeEnabled(false),
     _scopeHasArrived(false),
     _maintainLostRunTrigger(false) {
  _histo = (unsigned*)calloc(sizeOfHisto, sizeof(unsigned));
  strcpy(_runTimeConfigName, "");
  instance(this);
  printf("Epix10kaServer::Epix10kaServer() payload(%u)\n", _payloadSize);
  _dummy = (unsigned*)malloc(DummySize);
}

void  Pds::Epix10kaServer::setEpix10ka( int f ) {
  _myfd = f;
  fd(f);
  _cnfgrtr = new Pds::Epix10ka::Epix10kaConfigurator(fd(), _debug);
}

unsigned Pds::Epix10kaServer::configure(Epix10kaConfigType* config, bool forceConfig) {
  unsigned firstConfig = _resetOnEveryConfig || forceConfig;
  if (_firstconfig == true) {
    _firstconfig = false;
    firstConfig = 1;
    _cnfgrtr->runTimeConfigName(_runTimeConfigName);
    _cnfgrtr->maintainLostRunTrigger(_maintainLostRunTrigger);
    printf("Epix10kaServer::configure making new configurator %p, firstConfig %u\n", _cnfgrtr, firstConfig);
  }
  _cnfgrtr->fiberTriggering(fiberTriggering());
  _xtcTop.extent = sizeof(Xtc);
  _xtcSamplr.extent = 0;
  unsigned c = flushInputQueue(fd());

  if ((_configureResult = _cnfgrtr->configure(config, firstConfig))) {
    printf("Epix10kaServer::configure failed 0x%x, first %u\n", _configureResult, firstConfig);
  } else {
    _payloadSize = Epix10kaDataType::_sizeof(*config);
    if (_processorBuffer) free(_processorBuffer);
    _processorBuffer = (char*) calloc(1, _payloadSize);
    printf("Epix10kaServer::configure allocated processorBuffer size %u, firstConfig %u\n", _payloadSize, firstConfig);
    if (!_processorBuffer) {
      printf("Epix10kaServer::configure FAILED to allocated processor buffer!!!\n");
      return 0xdeadbeef;
    }
    _xtcEpix.extent = (_payloadSize * _elements) + sizeof(Xtc);
    _xtcTop.extent += _xtcEpix.extent;
    if ((_scopeEnabled = config->scopeEnable())) {
      _samplerConfig = new EpixSamplerConfigType(
          1,  // version
          config->epixRunTrigDelay(),
          config->epixRunTrigDelay() + 20,
          config->dacSetting(),
          config->adcClkHalfT(),
          config->adcPipelineDelay0(),
          config->digitalCardId0(),
          config->digitalCardId1(),
          config->analogCardId0(),
          config->analogCardId1(),
          2,  // number of Channels
          config->scopeTraceLength(),
          config->baseClockFrequency(),
          0  // testPatterEnable
      );
      _xtcSamplr.extent = EpixSamplerDataType::_sizeof(*_samplerConfig) + sizeof(Xtc);
      _xtcTop.extent += _xtcSamplr.extent;
      _xtcConfig.extent = sizeof( EpixSamplerConfigType) + sizeof(Xtc);
      if (!_scopeBuffer) _scopeBuffer = (unsigned*)calloc(_xtcSamplr.sizeofPayload(), 1);
    } else {
      if (_samplerConfig) {
        delete(_samplerConfig);
        _samplerConfig = 0;
      }
      _xtcSamplr.extent = 0;
      if (_scopeBuffer) {
        free(_scopeBuffer);
        _scopeBuffer = 0;
      }
    }
    if (_cnfgrtr->G3Flag()) {
      if (fiberTriggering()) {
        if  (_syncSlave == 0) {
          _syncSlave = new Pds::Epix10kaSyncSlave(_partition, _sync_task, this);
        }
        _g3sync = true;
      } else {
        _g3sync = false;
        if (_syncSlave) {
          delete _syncSlave;
          _syncSlave = 0;
        }
      }
    }

    printf("Epix10kaServer::configure _elements(%u) _payloadSize(%u) _xtcTop.extent(%u) _xtcEpix.extent(%u) firstConfig(%u)\n",
        _elements, _payloadSize, _xtcTop.extent, _xtcEpix.extent, firstConfig);

    _firstFetch = true;
    _count = _elementsThisCount = 0;
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

void Pds::Epix10kaServer::die() {
    printf("Epix10kaServer::die has been called !!!!!!!\n");
}

void Pds::Epix10kaServer::dumpFrontEnd() {
  if (_cnfgrtr) _cnfgrtr->dumpFrontEnd();
  else printf("Epix10kaServer::dumpFrontEnd() found nil configurator\n");
}

static unsigned* procHisto = (unsigned*) calloc(1000, sizeof(unsigned));

void Epix10kaServer::process(char* d) {
  Epix10kaDataType* e = (Epix10kaDataType*) _processorBuffer;
  timespec end;
  timespec start;
  clock_gettime(CLOCK_REALTIME, &start);
  Epix10kaDataType* o = (Epix10kaDataType*) d;

  //  header
  memcpy(o, e, sizeof(Epix10kaDataType));

  //  frame data
  unsigned nrows   = _cnfgrtr->configuration().numberOfReadableRowsPerAsic();
  unsigned colsize = _cnfgrtr->configuration().numberOfColumns()*sizeof(uint16_t);
  ndarray<const uint16_t,2> iframe = e->frame(_cnfgrtr->configuration());
  ndarray<const uint16_t,2> oframe = o->frame(_cnfgrtr->configuration());
  for(unsigned i=0; i<nrows; i++) {
    memcpy(const_cast<uint16_t*>(&oframe(nrows+i+0,0)), &iframe(2*i+0,0), colsize);
    memcpy(const_cast<uint16_t*>(&oframe(nrows-i-1,0)), &iframe(2*i+1,0), colsize);
  }

  unsigned tsz = reinterpret_cast<const uint8_t*>(e) + e->_sizeof(_cnfgrtr->configuration()) - reinterpret_cast<const uint8_t*>(iframe.end());
  memcpy(const_cast<uint16_t*>(oframe.end()), iframe.end(), tsz);

  clock_gettime(CLOCK_REALTIME, &end);
  long long unsigned diff = timeDiff(&end, &start);
  diff += 50000;
  diff /= 100000;
  if (diff > 1000-1) diff = 1000-1;
  procHisto[diff] += 1;
}

void Pds::Epix10kaServer::enable() {
  if (usleep(10000)<0) perror("Epix10kaServer::enable ulseep failed\n");
  _cnfgrtr->enableExternalTrigger(true);
  flushInputQueue(fd());
  _firstFetch = true;
  _countBase = 0;
  _ignoreFetch = _g3sync;
  if (_debug & 0x20) printf("Epix10kaServer::enable\n");
}

void Pds::Epix10kaServer::disable() {
  _ignoreFetch = true;
  if (_cnfgrtr) {
    _cnfgrtr->enableExternalTrigger(false);
    flushInputQueue(fd());
    if (usleep(10000)<0) perror("Epix10kaServer::disable ulseep 1 failed\n");
    if (_debug & 0x20) printf("Epix10kaServer::disable\n");
    dumpFrontEnd();
    printHisto(true);
  } else {
    printf("Epix10kaServer::disable() found nil configurator, so did not disable\n");
  }
}

void Pds::Epix10kaServer::runTimeConfigName(char* name) {
  if (name) strcpy(_runTimeConfigName, name);
  printf("Pds::Epix10kaServer::runTimeConfigName(%s)\n", name);
}

unsigned Pds::Epix10kaServer::unconfigure(void) {
  unsigned c = flushInputQueue(fd());
  if (c) printf("Epix10kaServer::unconfigure flushed %u event%s\n", c, c>1 ? "s" : "");
  return 0;
}

int Pds::Epix10kaServer::fetch( char* payload, int flags ) {
   int ret = 0;
   DmaReadData       pgpCardRx;
   unsigned        offset = 0;
   enum {Ignore=-1};

//   printf("Epix10kaServer got here 1, _ignoreFetch is %s\n", _ignoreFetch ? "true" : "false");
   if (_ignoreFetch) {
     flushInputQueue(fd(), false);
     return Ignore;
   }

//   printf("Epix10kaServer got here 2, _configured is %s\n", _configured ? "true" : "false");
   if (_configured == false)  {
     flushInputQueue(fd(), false);
     return Ignore;
   }

   _xtcTop.damage = 0;
   _xtcEpix.damage = 0;

   _elementsThisCount = 0;

   if (_debug & 3) printf("Epix10kaServer::fetch called\n");

   pgpCardRx.data   = (uint64_t)_processorBuffer;
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
       printf("Epix10kaServer::fetch exiting because of ERESTART\n");
       exit(-ERESTART);
     }
     perror ("Epix10kaServer::fetch pgpCard read error");
     ret =  Ignore;
   };

//   printf("Epix10kaServer got here 3, ret is %d, vc is %u\n", ret, pgpGetVc(pgpCardRx.dest));
   unsigned damageMask = 0;
   if (pgpCardRx.error) {
     if (pgpCardRx.error&DMA_ERR_FIFO)  damageMask |= 1;
     if (pgpCardRx.error&DMA_ERR_LEN)   damageMask |= 2;
     if (pgpCardRx.error&DMA_ERR_MAX)   damageMask |= 4;
     if (pgpCardRx.error&DMA_ERR_BUS)   damageMask |= 8;
   }

   if (pgpGetVc(pgpCardRx.dest) == Epix10ka::Epix10kaDestination::Oscilloscope) {
     if (damageMask) {
       _xtcSamplr.damage.increase(Pds::Damage::UserDefined);
       _xtcSamplr.damage.userBits(damageMask | 0xe0);
     } else {
       _xtcSamplr.damage = 0;
     }
     if (scopeEnabled())  {
       if (_scopeBuffer) {
         memcpy(_scopeBuffer, _processorBuffer, _xtcSamplr.sizeofPayload());
         _scopeHasArrived = true;
       }
     } else {
       printf("Epix10kaServer::fetch ignoring scope buffer %u when not enabled!\n", ++_neScopeCount);
     }
     return Ignore;
   }


   if (pgpGetVc(pgpCardRx.dest) == Epix10ka::Epix10kaDestination::Data) {

//     printf("Epix10kaServer got here 4, ret is %d\n", ret);

     Pds::Pgp::DataImportFrame* data = (Pds::Pgp::DataImportFrame*)(_processorBuffer);

     if ((ret > 0) && (ret < (int)_payloadSize)) {
       printf("Epix10kaServer::fetch() returning Ignore, ret was %d, looking for %u\n", ret, _payloadSize);
       if ((_debug & 4) || ret < 0) printf("\telementId(%u) frameType(0x%x) acqcount(0x%x) raw_count(%u) _elementsThisCount(%u) lane(%u) vc(%u)\n",
           data->elementId(), data->_frameType, data->acqCount(), data->frameNumber(), _elementsThisCount,
           pgpGetLane(pgpCardRx.dest)-Pds::Pgp::Pgp::portOffset(), pgpGetVc(pgpCardRx.dest));
       uint32_t* u = (uint32_t*)data;
       printf("\tDataHeader: "); for (int i=0; i<16; i++) printf("0x%x ", u[i]); printf("\n");
       ret = Ignore;
     }

     if (ret > (int) _payloadSize) printf("Epix10kaServer::fetch pgp read returned too much _payloadSize(%u) ret(%d)\n", _payloadSize, ret);

     if (damageMask) {
       damageMask |= 0xe0;
       _xtcEpix.damage.increase(Pds::Damage::UserDefined);
       _xtcEpix.damage.userBits(damageMask);
       printf("Epix10kaServer::fetch setting user damage 0x%x", damageMask);
       if (pgpCardRx.error&DMA_ERR_LEN) printf(", rxSize(%u), payloadSize(%u) ret(%d) offset(%u) (bytes)",
           (unsigned)pgpCardRx.size, _payloadSize, ret, offset);
       printf("\n");
     } else {
       unsigned oldCount = _count;
       _count = data->frameNumber() - 1;  // epix10ka starts counting at 1, not zero
       _fiducials = data->fiducials();    // for fiber triggering
//       printf("Epix10ka fetch frame number %u 0x%x fiducial 0x%x\n", _count, _count, _fiducials);
       if ((_debug & 5) || ret < 0) {
         printf("\telementId(%u) frameType(0x%x) acqcount(0x%x) _oldCount(%u) _count(%u) _elementsThisCount(%u) lane(%u) vc(%u)\n",
             data->elementId(), data->_frameType, data->acqCount(),  oldCount, _count, _elementsThisCount,
             pgpGetLane(pgpCardRx.dest)-Pds::Pgp::Pgp::portOffset(), pgpGetVc(pgpCardRx.dest));
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
//             printf("Epix10kaServer::fetch exceptional period %3lld, not %3u, frame %5u, frames since last %5u, ms since last %7.3f, ms/f %6.3f\n",
//             diff, peak, _count, _fetchesSinceLastException, _timeSinceLastException, (1.0*_timeSinceLastException)/_fetchesSinceLastException);
             printf("Epix10kaServer::fetch exceptional period %3lld, not %3u, frame %5u, frames since last %5u\n",
                 diff, peak, _count, _fetchesSinceLastException);
             _timeSinceLastException = 0;
             _fetchesSinceLastException = 0;
           }
         }
       } else {
         printf("Epix10kaServer::fetch Clock backtrack %f ms\n", diff / 1000000.0);
       }
//       if (g3sync() && (data->opCode() != (_lastOpCode + 1)%256)) {
//         printf("Epix10kaServer::fetch opCode mismatch last(%u) this(%u) on frame(0x%x)\n",
//             _lastOpCode, data->opCode(), _count);
//       }
       memcpy(&_lastTime, &_thisTime, sizeof(timespec));
     }
     _lastOpCode = data->opCode();
     offset = sizeof(Xtc);
     if (_scopeHasArrived) {
       memcpy(payload + offset, &_xtcSamplr, sizeof(Xtc));
       offset += sizeof(Xtc);
       memcpy(payload + offset, _scopeBuffer, _xtcSamplr.sizeofPayload());
       offset += _xtcSamplr.sizeofPayload();
       _scopeHasArrived = false;
     }
     memcpy(payload + offset, &_xtcEpix, sizeof(Xtc));
     offset += sizeof(Xtc);
     process(payload + offset);
     _xtcTop.extent = offset + _payloadSize;
     memcpy(payload, &_xtcTop, sizeof(Xtc));
   } else {  // wrong vc for us
     return Ignore;
   }
   if (ret > 0) {
     ret += offset;
   }
   if (_debug & 5) printf(" returned %d\n", ret);
   return ret;
}

bool Epix10kaServer::more() const {
  bool ret = false;
  if (_debug & 2) printf("Epix10kaServer::more(%s)\n", ret ? "true" : "false");
  return ret;
}

unsigned Epix10kaServer::offset() const {
  unsigned ret = _elementsThisCount == 1 ? 0 : sizeof(Xtc) + _payloadSize * (_elementsThisCount-1);
  if (_debug & 2) printf("Epix10kaServer::offset(%u)\n", ret);
  return (ret);
}

unsigned Epix10kaServerCount::count() const {
  if (_debug & 2) printf( "Epix10kaServerCount::count(%u)\n", _count + _offset);
  return _count + _offset;
}

unsigned Epix10kaServerSequence::fiducials() const {
  if (_debug & 2) printf( "Epix10kaServerSequence::fiducials(%u)\n", _fiducials);
  return _fiducials;
}

unsigned Epix10kaServer::flushInputQueue(int f, bool flag) {
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
    printf("Epix10kaServer::flushInputQueue flushed %u scope buffers and %u other buffers\n", scopeCount, count-scopeCount);
  }
  return count;
}

void Epix10kaServer::clearHisto() {
  for (unsigned i=0; i<sizeOfHisto; i++) {
    _histo[i] = 0;
  }
}

void Epix10kaServer::printHisto(bool c) {
  unsigned histoSum = 0;
  printf("Epix10kaServer event fetch periods\n");
  for (unsigned i=0; i<sizeOfHisto; i++) {
    if (_histo[i]) {
      printf("\t%3u ms   %8u\n", i, _histo[i]);
      histoSum += _histo[i];
      if (c) _histo[i] = 0;
    }
  }
  printf("\tHisto Sum was %u\n", histoSum);
  histoSum = 0;
  printf("Epix10kaServer unshuffle event periods\n");
  for (unsigned i=0; i<1000; i++) {
    if (procHisto[i]) {
      printf("\t%3u 0.1ms   %8u\n", i, procHisto[i]);
      histoSum += procHisto[i];
      if (c) procHisto[i] = 0;
    }
  }
  printf("\tProchisto (events) Sum was %u\n", histoSum);
}
