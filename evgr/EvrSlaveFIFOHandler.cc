#include "pds/evgr/EvrSlaveFIFOHandler.hh"
#include "pds/evgr/EvrFifoServer.hh"
#include "pds/xtc/InDatagram.hh"
#include "pds/utility/Appliance.hh"
#include "pds/utility/Occurrence.hh"
#include "pds/utility/StreamPorts.hh"
#include "pds/service/Ins.hh"
#include "pds/collection/Route.hh"
#include "pds/service/Task.hh"
#include "pds/xtc/EvrDatagram.hh"
#include "evgr/evr/evr.hh"

#include <new>
#include <string.h>
#include <signal.h>
#include <byteswap.h>

using namespace Pds;

static unsigned _nPrint;

static bool evrHasEvent(Evr& er);

/*
 * Signal handler, for processing the incoming event codes, and providing interfaces for
 *   retrieving L1 data from the L1Xmitter object
 * The Slave EVR process is indicated by L1Xmitter::enable.  The master is responsible
 * for sending the EvrDatagram to the other segment levels, generating the sw triggers,
 * adding the FIFO data to the L1Accept datagram, and counting events for calibration cycles.
 * All EVR processes configure the
 * EVRs to generate hardware triggers.  The slave EVR processes only need verify that
 * their FIFO data matches the timestamp of the L1Accept generated by the master.
 */
EvrSlaveFIFOHandler::EvrSlaveFIFOHandler(
           Evr&       er,
           Appliance& app,
           EvrFifoServer& srv,
           unsigned   partition,
           int        iMaxGroup,
           unsigned   module,
           Task*      task,
           Task*      sync_task) :
  _er                 (er),
  _module             (module),
  _app                (app),
  _srv                (srv),
  _outlet             (sizeof(EvrDatagram), 0, Ins   (Route::interface())),
  _evtCounter         (0),
  _uMaskReadout       (0),
  _pEvrConfig         (NULL),
  _rdptrSlave         (0),
  _wrptrSlave         (0),
  _rdptrMaster        (0),
  _wrptrMaster        (0),
  _sync               (*this, er, partition, task, sync_task),
  _tr                 (0),
  _occPool            (sizeof(UserMessage),8),
  _outOfOrder         (false),
  _bEnabled           (false),
  _bShowFirst         (false)
{
  memset( _lEventCodeState, 0, sizeof(_lEventCodeState) );
  _lSegEvtCounter.resize(1+iMaxGroup, 0);
  for (int iGroup=0; iGroup <= iMaxGroup; ++iGroup)
    _ldst.push_back(StreamPorts::event(partition,Level::Segment, iGroup, _module));
}

EvrSlaveFIFOHandler::~EvrSlaveFIFOHandler()
{
}

void EvrSlaveFIFOHandler::fifo_event(const FIFOEvent& fe)
{
  if ( _sync.handle(fe) )
    return;

  /*
  if ( _bShowFirst )
    {
      timespec ts;
      clock_gettime(CLOCK_REALTIME, &ts);
      printf("EvrSlaveFIFOHandler first fifo event at %d.%09d\n",
       int(ts.tv_sec), int(ts.tv_nsec));
      bShowFirst = false;
    }
  */

  if ( _bEnabled == false )
    {
      printf("EvrSlaveFIFOHandler::fifo_event(): during Disabled, vector %d code %d fiducial 0x%x last 0x%x timeLow 0x%x\n",
             _evtCounter, fe.EventCode, fe.TimestampHigh, _lastFiducial, fe.TimestampLow);
    }

  if (fe.EventCode == TERMINATOR) {
    // TERMINATOR will not be stored in the event code list
    if (_uMaskReadout || _ncommands)
      startCommandAndQueueEvent(fe, false);
    return;
  }

  uint32_t        uEventCode = fe.EventCode;
  EventCodeState& codeState  = _lEventCodeState[uEventCode];

  if ( codeState.iDefReportWidth == 0 )// not an interesting event
    ;
  else if ( codeState.uMaskReadout )
  {
    _lastFiducial  = fe.TimestampHigh;
    _uMaskReadout |= codeState.uMaskReadout;
  }
  else if (codeState.bCommand)
  {
    if (_ncommands < giMaxCommands)
      _commands[_ncommands++] = fe.EventCode;
    else
      printf("EvrSlaveFIFOHandler::fifo_event(): Dropped command %d\n",fe.EventCode);
  }
}

InDatagram* EvrSlaveFIFOHandler::l1accept(InDatagram* in)
{
  InDatagram* out = in;

  if (_wrptrMaster != _rdptrMaster && (_wrptrMaster-_rdptrMaster) % QSize == 0)
  {
    if (++_numMasterQFull == 1)
      printf("EvrSlaveFIFOHandler::l1accept(): masterQ full!\n");
    ++_rdptrMaster;
  }
  else
  {
    if (_numMasterQFull > 1)
      printf("EvrSlaveFIFOHandler::l1accept(): masterQ was full for %d times!\n", _numMasterQFull);
    _numMasterQFull = 0;
  }
  _masterQ[_wrptrMaster%QSize] = in->datagram().seq;
  //printf("Wr master [%04d, r %04d] fiducial 0x%x\n", _wrptrMaster, _rdptrMaster, in->datagram().seq.stamp().fiducials());//!!!debug
  unsigned prevWrtMaster = _wrptrMaster++;

  startL1Accept(true, prevWrtMaster);

  //if (out->datagram().xtc.damage.value()) {
  //}
  //else {
  //  unsigned slave_fiducial =
  //    *reinterpret_cast<uint32_t*>(in->datagram().xtc.payload()+
  //                                 in->datagram().xtc.sizeofPayload()-sizeof(uint32_t));

  //  out->datagram().xtc.extent = sizeof(Xtc);

  //  // check counter,fiducial match => set damage if failed
  //  // 2013-05-17 Comment out the fiducial check until CXI problems are better understood
  //  // 2013-07-08 Seems we may be ignoring an error in XPP by not making this check.  Put it back.
  //  //            I find these errors originate from the slave handling the disable "eventcode"
  //  //            ~8.5 msec too late.
  //  if (slave_fiducial != in->datagram().seq.stamp().fiducials()) {
  //    bool needOccurrance = !_outOfOrder; // only send occurrences on 1st detection (reduce user messages)
  //    _outOfOrder = true;
  //    printf("EvrSlaveFIFOHandler out of order on %x : slave %x  buffer %x  wrptr %x  rdptr %x\n",
  //           in->datagram().seq.stamp().fiducials(),
  //           slave_fiducial, slaveEvent.ts.fiducials(), _wrptrSlave, _rdptrSlave);

  //    if (needOccurrance && (_occPool.numberOfFreeObjects()>=2)) {
  //      Occurrence* occ = new (&_occPool) Occurrence(OccurrenceId::ClearReadout);
  //      _app.post(occ);

  //      UserMessage* umsg = new (&_occPool) UserMessage;
  //      umsg->append("Slave EVR out-of-order.  Reconfigure.");
  //      _app.post(umsg);
  //    }
  //  }
  //}

  if (_outOfOrder)
    out->datagram().xtc.damage.increase(Pds::Damage::OutOfOrder);
  if (_bEventSkipped)
  {
    _bEventSkipped = false;
    out->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
  }

  return out;
}

Transition* EvrSlaveFIFOHandler::enable      (Transition* tr)
{
  clear();
  _wrptrSlave   = _rdptrSlave   = 0;
  _wrptrMaster  = _rdptrMaster  = 0;
  _numMasterQFull = _numSlaveQFull = 0;
  _bEnabled   = true;
  _bShowFirst = true;
  _nPrint = 20;

  //!!!debug
  printf("\nEnable  Trigger ");
  for (int i=0; i < (int) _lSegEvtCounter.size(); ++i)
    printf(" [%d] %d ", i, _lSegEvtCounter[i]);
  printf("\n");
  return tr;
}

Transition* EvrSlaveFIFOHandler::disable     (Transition* tr)
{
  if (_numSlaveQFull > 1)
    printf("EvrSlaveFIFOHandler::disable(): slaveQ was Full for %d times!\n", _numSlaveQFull);
  if (_numMasterQFull > 1)
    printf("EvrSlaveFIFOHandler::disable(): masterQ was full for %d times!\n", _numMasterQFull);
  _tr = tr;

  return (Transition*)Appliance::DontDelete;
}

void EvrSlaveFIFOHandler::get_sync()
{
  _sync.enable();
}

void EvrSlaveFIFOHandler::release_sync()
{
  clear();
  //  _sem.give();   // sometimes we get one less FIFO event than the master
  _bEnabled = false;

  //!!!debug
  printf("\nRelease_sync after clear: slave r %d w %d  master r %d w %d\n", _rdptrSlave, _wrptrSlave, _rdptrMaster, _wrptrMaster);
  printf("  Trigger ");
  for (int i=0; i < (int) _lSegEvtCounter.size(); ++i)
    printf(" [%d] %d ", i, _lSegEvtCounter[i]);
  printf("\n");

  _app.post(_tr);
}

void        EvrSlaveFIFOHandler::set_config  (const EvrConfigType* pEvrConfig)
{
  _pEvrConfig = pEvrConfig;

  memset( _lEventCodeState, 0, sizeof(_lEventCodeState) );

  unsigned int uEventIndex = 0;
  for ( ; uEventIndex < _pEvrConfig->neventcodes(); uEventIndex++ )
    {
      const EventCodeType& eventCode = _pEvrConfig->eventcodes()[uEventIndex];
      if ( eventCode.code() >= guNumTypeEventCode )
        {
          printf( "EvrSlaveFIFOHandler::setEvrConfig(): event code out of range: %d\n", eventCode.code() );
          continue;
        }

      EventCodeState& codeState = _lEventCodeState[eventCode.code()];
      codeState.configure(eventCode);
      //codeState.uMaskReadout    = eventCode.isReadout   ();
      //codeState.bCommand        = eventCode.isCommand   ();
      //codeState.iDefReportDelay = eventCode.reportDelay ();
      //if (eventCode.isLatch())
      //  codeState.iDefReportWidth = -eventCode.releaseCode();
      //else
      //  codeState.iDefReportWidth = eventCode.reportWidth ();

      //printf("EventCode %d  readout %c  command %c  latch %c  delay %d  width %d\n",
      //       eventCode.code(),
      //       eventCode.isReadout() ? 't':'f',
      //       eventCode.isCommand() ? 't':'f',
      //       eventCode.isLatch  () ? 't':'f',
      //       eventCode.reportDelay(),
      //       eventCode.reportWidth());
    }
  _outOfOrder     = false;
  _bEventSkipped  = false;
}

Transition* EvrSlaveFIFOHandler::config      (Transition* tr)
{
  clear();
  _evtCounter = 0;
  _lSegEvtCounter.assign(_lSegEvtCounter.size(), 0);
  return tr;
}

Transition* EvrSlaveFIFOHandler::endcalib    (Transition* tr)
{
  return tr;
}


void EvrSlaveFIFOHandler::startCommandAndQueueEvent(const FIFOEvent& fe, bool bEvrDataIncomplete)
{
  static pthread_t        tid   = -1;
  static pthread_mutex_t  mutex   = PTHREAD_MUTEX_INITIALIZER;

  if ( pthread_equal(tid, pthread_self()) )
  {
    printf("EvrSlaveFIFOHandler::startL1Accept(): Re-entry call for vector %d fiducial 0x%x last 0x%x timeLow 0x%x code %d\n",
           _evtCounter, fe.TimestampHigh,
           _lastFiducial, fe.TimestampLow, fe.EventCode );
    return;
  }
  tid = pthread_self();

  if ( pthread_mutex_lock(&mutex) )
  {
    printf("EvrSlaveFIFOHandler::startL1Accept(): pthread_mutex_lock() failed\n");
    return;
  }

  if ( !_uMaskReadout && !_ncommands )
  {
    if ( pthread_mutex_unlock(&mutex) )
      printf( "EvrSlaveFIFOHandler::startL1Accept(): pthread_mutex_unlock() failed\n" );

    printf("EvrSlaveFIFOHandler::startL1Accept(): No readout/commands detected. Possibly called by two threads.\n");
    tid = -1;
    return;
  }

  /*
  if (fe.TimestampHigh == 0 && uFiducialPrev < 0x1fe00) // Illegal fiducial wrap-around
    {
      printf("EvrSlaveFIFOHandler::startL1Accept(): [%d] vector %d fiducial 0x%x prev 0x%x Incomplete %c last 0x%x timeLow 0x%x code %d\n",
             uNumBeginCalibCycle, _evtCounter, fe.TimestampHigh, uFiducialPrev, (bEvrDataIncomplete?'Y':'n'),
             _lastFiducial, fe.TimestampLow, fe.EventCode );
    }
  */

  if (_ncommands) {
    timespec  ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    ClockTime   ctime   (ts.tv_sec, ts.tv_nsec);
    TimeStamp   stamp   (fe.TimestampLow, fe.TimestampHigh, _evtCounter);
    Sequence    seq     (Sequence::Occurrence, TransitionId::Unknown, ctime, stamp);
    EvrDatagram datagram(seq, _evtCounter, _ncommands);
    for (int iGroup = 0; iGroup < (int) _ldst.size(); ++iGroup) {
      datagram.evr = _lSegEvtCounter[iGroup];
      _outlet.send((char *) &datagram,
                   _commands, _ncommands, _ldst[iGroup]);
    }
    _ncommands = 0;
  }

  bool      bStartL1Accept = false;
  unsigned  prevWrtSlave   = 0;
  if (_uMaskReadout)
  {
    _uMaskReadout             |= 0x1; // Readout group 0 is always triggered
    if (_wrptrSlave != _rdptrSlave && (_wrptrSlave-_rdptrSlave) % QSize == 0)
    {
      if (++_numSlaveQFull == 1)
        printf("EvrSlaveFIFOHandler::startL1Accept(): slaveQ full!\n");
      slaveQAdvanceUntil(_rdptrSlave);
    }
    else
    {
      if (_numSlaveQFull > 1)
        printf("EvrSlaveFIFOHandler::startL1Accept(): slaveQ was full for %d times!\n", _numSlaveQFull);
      _numSlaveQFull = 0;
    }
    SlaveEvent slaveEvent      = { TimeStamp(fe.TimestampLow, fe.TimestampHigh, _evtCounter), _uMaskReadout };
    _slaveQ[_wrptrSlave%QSize] = slaveEvent;

    //printf("Wr slave [%04d, r %04d] fiducial 0x%x  count %d  mask 0x%x\n", _wrptrSlave, _rdptrSlave, fe.TimestampHigh, _evtCounter, _uMaskReadout);//!!!debug

    prevWrtSlave = _wrptrSlave++;
    _uMaskReadout  = 0;
    bStartL1Accept = true;
    //_srv.post(_evtCounter++,fe.TimestampHigh);//!!!tmp
  }

  if ( pthread_mutex_unlock(&mutex) )
    printf( "EvrSlaveFIFOHandler::startL1Accept(): pthread_mutex_unlock() failed\n" );

  if (bStartL1Accept)
    startL1Accept(false, prevWrtSlave);

  tid = -1;
}

int EvrSlaveFIFOHandler::startL1Accept(bool bMasterEvent, unsigned prevWrt)
{
  if (_rdptrMaster == _wrptrMaster || _rdptrSlave == _wrptrSlave) // Either master or slave queue is empty
    return 0;

  static pthread_mutex_t  mutex   = PTHREAD_MUTEX_INITIALIZER;
  if ( pthread_mutex_lock(&mutex) )
  {
    printf("EvrSlaveFIFOHandler::startL1Accept(): pthread_mutex_lock() failed\n");
    return 1;
  }

  bool        bEventMatched = false;
  Sequence    masterSeq;
  unsigned    uMaskReadout    = 0;
  unsigned    prevrdptrSlave  = _rdptrSlave;
  unsigned    prevrdptrMaster = _rdptrMaster;
  if (bMasterEvent)
  {
    const unsigned fiducial = _masterQ[prevWrt%QSize].stamp().fiducials();
    for (unsigned rdptr = _rdptrSlave; rdptr != _wrptrSlave; ++rdptr)
      if (_slaveQ[rdptr%QSize].ts.fiducials() == fiducial)
      {
        masterSeq     = _masterQ[prevWrt%QSize];
        uMaskReadout  = _slaveQ [rdptr  %QSize].uMaskReadout;
        _rdptrMaster  = prevWrt+1;
        slaveQAdvanceUntil(rdptr);
        bEventMatched = true;
        break;
      }
  }
  else
  {
    const unsigned fiducial = _slaveQ[prevWrt%QSize].ts.fiducials();
    for (unsigned rdptr = _rdptrMaster; rdptr != _wrptrMaster; ++rdptr)
      if (_masterQ[rdptr%QSize].stamp().fiducials() == fiducial)
      {
        masterSeq     = _masterQ[rdptr  %QSize];
        uMaskReadout  = _slaveQ [prevWrt%QSize].uMaskReadout;
        _rdptrMaster  = rdptr+1;
        slaveQAdvanceUntil(prevWrt);
        bEventMatched = true;
        break;
      }
  }

  bool bSlaveSkip   = (prevrdptrSlave +1 != _rdptrSlave  && prevrdptrSlave  != _rdptrSlave);
  bool bMasterSkip  = (prevrdptrMaster+1 != _rdptrMaster && prevrdptrMaster != _rdptrMaster);
  if (bSlaveSkip || bMasterSkip)
  {
    _bEventSkipped = true;
    if (bMasterEvent)
      printf("\nMatch from Master,  rdslave %d -> %d [w %d]  rdmaster %d (%d) -> %d [w %d]\n",
        prevrdptrSlave, _rdptrSlave, _wrptrSlave, prevrdptrMaster, prevWrt, _rdptrMaster, _wrptrMaster);
    else
      printf("\nMatch from Slave,  rdslave %d (%d) -> %d [w %d]  rdmaster %d -> %d [w %d]:\n",
        prevrdptrSlave, prevWrt, _rdptrSlave, _wrptrSlave, prevrdptrMaster, _rdptrMaster, _wrptrMaster);
    printf("  Trigger ");
    for (int i=0; i < (int) _lSegEvtCounter.size(); ++i)
      printf(" [%d] %d ", i, _lSegEvtCounter[i]);
    printf("\n");

    if (prevrdptrSlave >= 3)
      printf("SlaveQ [%d] 0x%x\n", prevrdptrSlave-3, _slaveQ[(prevrdptrSlave-3)%QSize].ts.fiducials());
    if (prevrdptrSlave >= 2)
      printf("SlaveQ [%d] 0x%x\n", prevrdptrSlave-2, _slaveQ[(prevrdptrSlave-2)%QSize].ts.fiducials());
    if (prevrdptrSlave != 0)
      printf("SlaveQ [%d] 0x%x\n", prevrdptrSlave-1, _slaveQ[(prevrdptrSlave-1)%QSize].ts.fiducials());
    if (!bSlaveSkip)
      printf("SlaveQ [%d] 0x%x\n", prevrdptrSlave  , _slaveQ[(prevrdptrSlave  )%QSize].ts.fiducials());
    if (bSlaveSkip)
    {
      unsigned numSkip = (((_rdptrSlave - prevrdptrSlave - 1) % QSize) + QSize) % QSize;
      printf("SlaveQ skipped %d event(s).\n", numSkip);
      for (unsigned i=0; i<numSkip; ++i)
        printf("  [%d] 0x%x\n", prevrdptrSlave+i, _slaveQ[(prevrdptrSlave+i)%QSize].ts.fiducials());
      printf("SlaveQ [%d] 0x%x\n", _rdptrSlave-1, _slaveQ[(_rdptrSlave-1)%QSize].ts.fiducials());
    }
    if (_rdptrSlave != _wrptrSlave)
      printf("SlaveQ [%d] 0x%x\n", _rdptrSlave, _slaveQ[_rdptrSlave%QSize].ts.fiducials());

    if (prevrdptrMaster >= 3)
      printf("MasterQ [%d] 0x%x\n", prevrdptrMaster-3, _masterQ[(prevrdptrMaster-3)%QSize].stamp().fiducials());
    if (prevrdptrMaster >= 2)
      printf("MasterQ [%d] 0x%x\n", prevrdptrMaster-2, _masterQ[(prevrdptrMaster-2)%QSize].stamp().fiducials());
    if (prevrdptrMaster != 0)
      printf("MasterQ [%d] 0x%x\n", prevrdptrMaster-1, _masterQ[(prevrdptrMaster-1)%QSize].stamp().fiducials());
    if (!bMasterSkip)
      printf("MasterQ [%d] 0x%x\n", prevrdptrMaster  , _masterQ[(prevrdptrMaster  )%QSize].stamp().fiducials());
    if (bMasterSkip)
    {
      unsigned numSkip = (((_rdptrMaster - prevrdptrMaster - 1) % QSize) + QSize) % QSize;
      printf("MasterQ skipped %d event(s).\n", numSkip);
      for (unsigned i=0; i<numSkip; ++i)
        printf("  [%d] 0x%x\n", prevrdptrMaster+i, _masterQ[(prevrdptrMaster+i)%QSize].stamp().fiducials());
      printf("MasterQ [%d] 0x%x\n", _rdptrMaster-1, _masterQ[(_rdptrMaster-1)%QSize].stamp().fiducials());
    }
    if (_rdptrMaster != _wrptrMaster)
      printf("MasterQ [%d] 0x%x\n", _rdptrMaster, _masterQ[_rdptrMaster%QSize].stamp().fiducials());
  }

  if (bEventMatched)
  {
    EvrDatagram datagram(masterSeq, _evtCounter++, 0);
    datagram.setL1AcceptEnv(uMaskReadout);
    unsigned int uGroupBit = 1;
    for (int iGroup = 0; iGroup < (int) _ldst.size(); ++iGroup, uGroupBit <<= 1)
      if ( uMaskReadout & uGroupBit ) {
        //printf("sending L1 trigger for group %d counter %d\n", iGroup, _lSegEvtCounter[iGroup]);//!!!debug
        datagram.evr = _lSegEvtCounter[iGroup]-1;
        _outlet.send((char *) &datagram,
                     _commands, 0, _ldst[iGroup]);
      }
  }

  if ( pthread_mutex_unlock(&mutex) )
    printf( "EvrSlaveFIFOHandler::startL1Accept(): pthread_mutex_unlock() failed\n" );

  return bEventMatched? 0 : 2;
}

void EvrSlaveFIFOHandler::clear()
{
  const int iMaxCheck = 10;
  int       iCheck    = 0;
  while ( evrHasEvent(_er) && ++iCheck <= iMaxCheck )
    { // sleep for 2 millisecond to let signal handler process FIFO events
      timeval timeSleepMicro = {0, 2000}; // 2 milliseconds
      select( 0, NULL, NULL, NULL, &timeSleepMicro);
    }

  if (iCheck > iMaxCheck)
    {
      printf("EvrSlaveFIFOHandler::clear quit after %d checks\n",iCheck);
    }

  if (_uMaskReadout || _ncommands)
    {
      FIFOEvent fe;
      fe.TimestampHigh    = _lastFiducial;
      fe.TimestampLow     = 0;
      fe.EventCode        = TERMINATOR;

      /*
      if (fe.TimestampHigh == 0 && uFiducialPrev < 0x1fe00) // Illegal fiducial wrap-around
        printf("EvrSlaveFIFOHandler::clear(): [%d] Call startL1Accept() with vector %d fiducial 0x%x prev 0x%x last 0x%x timeLow 0x%x\n",
               uNumBeginCalibCycle, _evtCounter, fe.TimestampHigh, uFiducialPrev, _lastFiducial, fe.TimestampLow);
      */

      startCommandAndQueueEvent(fe, true);
    }
}

void EvrSlaveFIFOHandler::slaveQAdvanceUntil(unsigned rdptrUntil)
{
  static pthread_mutex_t  mutex   = PTHREAD_MUTEX_INITIALIZER;
  if ( pthread_mutex_lock(&mutex) )
  {
    printf("EvrSlaveFIFOHandler::slaveQAdvanceUntil(): pthread_mutex_lock() failed\n");
    return;
  }

  while (true)
  {
    if (_wrptrSlave == _rdptrSlave)
    {
      printf("!!! EvrSlaveFIFOHandler::slaveQAdvanceUntil(): slaveQ is empty!\n");
      break;
    }

    SlaveEvent  slaveEvent = _slaveQ[_rdptrSlave%QSize];
    unsigned int uGroupBit = 1;
    for (int iGroup = 0; iGroup < (int) _lSegEvtCounter.size(); ++iGroup, uGroupBit <<= 1)
      if ( (slaveEvent.uMaskReadout & uGroupBit) != 0 )
        ++_lSegEvtCounter[iGroup];
    if (rdptrUntil == _rdptrSlave++)
      break;
  }

  if ( pthread_mutex_unlock(&mutex) )
    printf( "EvrSlaveFIFOHandler::slaveQAdvanceUntil(): pthread_mutex_unlock() failed\n" );
}

// check if evr has any unprocessed fifo event
bool evrHasEvent(Evr& er)
{
  uint32_t& uIrqFlagOrg = *(uint32_t*) ((char*) &er + 8);
  uint32_t  uIrqFlagNew = be32_to_cpu(uIrqFlagOrg);

  if ( uIrqFlagNew & EVR_IRQFLAG_EVENT)
    return true;
  else
    return false;
}
