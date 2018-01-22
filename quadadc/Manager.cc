#include "pds/quadadc/Manager.hh"
#include "pds/quadadc/Server.hh"
#include "hsd/Module.hh"
//#include "pds/quadadc/StatsTimer.hh"
//#include "pds/quadadc/QABase.hh"
//#include "pds/quadadc/EpicsCfg.hh"

//#include "pds/config/TriggerConfigType.hh"
#include "pds/config/CfgClientNfs.hh"
#include "pds/utility/Appliance.hh"
#include "pds/service/GenericPool.hh"
#include "pds/service/Routine.hh"
#include "pds/service/Task.hh"
#include "pds/service/Timer.hh"

#include "pds/vmon/VmonServerManager.hh"
#include "pds/mon/MonGroup.hh"
#include "pds/mon/MonDescScalar.hh"
#include "pds/mon/MonEntryScalar.hh"

//#include "pds/epicstools/PVWriter.hh"
//using Pds_Epics::PVWriter;

#include "pdsdata/psddl/generic1d.ddl.h"
#include "hsd/Module.hh"
#include <string>
#include <sstream>
#include <cmath>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "pds/config/QuadAdcConfigType.hh"

using namespace Pds;
using Pds::HSD::Module;
using Pds::QuadAdc::Manager;
using Pds::QuadAdc::Server;

static Manager* _this = 0;

static void s_halt(int)
{
  if (_this)
    _this->halt();
  exit(0);
}


Manager::Manager(Pds::HSD::Module&           dev,
		 Server&           srv,
		 CfgClientNfs&     cfg,
		 int               testpattern) :
  _dev(dev),
  _srv(srv),
  _cfg(cfg),
  //_qab(dev.base),
  _cbuff(new char[Pds::Generic1D::ConfigV0(8,0,0,0,0)._sizeof()]),
  _testpattern(testpattern),
  _occPool(sizeof(UserMessage),1)
{
  // Register interrupt handler for graceful shutdown
    _this=this;
   struct sigaction int_action;

   int_action.sa_handler = s_halt;
   sigemptyset(&int_action.sa_mask);
   int_action.sa_flags = 0;
   int_action.sa_flags |= SA_RESTART;
   
   if (sigaction(SIGINT, &int_action, 0) > 0)
     printf("Couldn't set up SIGINT handler\n");
   if (sigaction(SIGKILL, &int_action, 0) > 0)
     printf("Couldn't set up SIGKILL handler\n");
   if (sigaction(SIGSEGV, &int_action, 0) > 0)
     printf("Couldn't set up SIGSEGV handler\n");
   if (sigaction(SIGABRT, &int_action, 0) > 0)
     printf("Couldn't set up SIGABRT handler\n");
   if (sigaction(SIGTERM, &int_action, 0) > 0)
     printf("Couldn't set up SIGTERM handler\n");

   _dev.init(); 
}

Manager::~Manager() {
  delete _cbuff;
}

Transition* Manager::transitions(Transition* tr) {

  _nerror = 0;

  switch(tr->id()) {
  case TransitionId::L1Accept:
    break;
  case TransitionId::Enable:
    //_qab.start();
    _dev.start();
    break;
  case TransitionId::Disable:
    //_qab.stop();
    _dev.stop();
    break;
  case TransitionId::Map:
    _cfg.initialize(reinterpret_cast<const Allocate&>(*tr).allocation());
    break;
  case TransitionId::BeginRun:
    break;
  case TransitionId::EndRun:
    break;
  case TransitionId::Configure:
    {
      int len     = _cfg.fetch( *tr,
                                _QuadAdcConfigType,
                                &_config,
                                sizeof(_config) );
    
      if (len <= 0) {
        printf("ConfigAction: failed to retrieve configuration "
               ": (%d) %s.  Applying default.\n",
               errno,
               strerror(errno) );
        _nerror += 1;
      }
   
    
      else {
        _cfgtc.extent     = sizeof(Xtc) + sizeof(QuadAdcConfigType);
      }
    }
    { 
      double   sr = _config.sampleRate();
      unsigned ns = _config.nbrSamples();
      unsigned in = _config.interleaveMode();
      //printf("SAMPLE RATE: %i\n", sr);
    
      UserMessage* msg = new (&_occPool) UserMessage;
      _nerror = 0; 
      
      unsigned length=0;
      if (ns%16 == 0) {
        //_qab.samples = ns; 
        length = ns;
      }
      else {
        //_qab.samples = (ns+15)&~0xf;
        length = (ns+15)&~0xf;
      }

      _dev.setAdcMux(_config.chanMask());

      unsigned ps = unsigned(1.25e9/sr + 0.5);
      printf("ps %u\n", ps);
      if (ps & ~0x3f) {
        printf("ConfigAction: sample rate out of range\n");
        _nerror++;
        msg->append("sample rate out of range");
      }

      double dl_calc = _config.delayTime()*119e-3;
      unsigned delay = unsigned(dl_calc+0.5); // 119 MHz ticks

      if(delay & ~0xfffff){
        printf("ConfigAction: delay out of range\n");
        _nerror += 1;
        msg->append("delay out of range\n");
      }

      //_qab.prescale = (ps & 0x3f) | (delay<<6);
      //_qab.acqSelect = ec & 0xff;
    
      _dev.sample_init(length, delay, ps);

      switch(_testpattern){
      case 0:
        _dev.enable_test_pattern(HSD::Module::Ramp);
        break;
      case 1:
        _dev.enable_test_pattern(HSD::Module::Flash11);
        break;
      case 3:
        _dev.enable_test_pattern(HSD::Module::Flash12);
        break;
      case 5:
        _dev.enable_test_pattern(HSD::Module::Flash16);
        break;
      case 8:
        _dev.enable_test_pattern(HSD::Module::DMA);
        break;
      default:
        _dev.disable_test_pattern();
      }
      //Ramp=0, Flash11=1, Flash12=3, Flash16=5, DMA=8

      _dev.trig_lcls(_config.evtCode());
  
      _dev.dumpBase();

      double period = (in==1) ? 0.2e-9 : 0.8e-9*double(ps);

      //
      //  Round delay to the nearest sub-sample period
      //  (limited by Generic1DConfig)
      //
      unsigned delay_ss = unsigned(_config.delayTime()*1.e-9/period + 0.5);

      const unsigned nch = _dev.ncards()*4;

      //  Create the Generic1DConfig
      unsigned config_length     [nch];
      unsigned config_sample_type[nch];
      int      config_offset     [nch];
      double   config_period     [nch];
      
      for(unsigned i=0; i<nch; i++) {
        config_length     [i] = ((in==0) | (_config.chanMask()&(1<<(i&3)))) ? ns : 0;
        config_sample_type[i] = Pds::Generic1D::ConfigV0::UINT16;
        config_offset     [i] = delay_ss;
        config_period     [i] = period;

        printf("Gen1DCfg ch %u: len %u  off %u  period %g\n",
               i, config_length[i], config_offset[i], config_period[i]);
      }

      new(_cbuff)
        Pds::Generic1D::ConfigV0(nch,
                                 config_length,
                                 config_sample_type,
                                 config_offset,
                                 config_period);
      if(_nerror){
        post(msg);
      }
      else {
        delete msg;
      }
    }
    break;
  default:
    break;
  }
  return tr;
}

InDatagram* Manager::events     (InDatagram* dg) {
  switch(dg->datagram().seq.service()) {
  case TransitionId::L1Accept:
    break;
  case TransitionId::Configure:
    {const Pds::Generic1D::ConfigV0& c = *reinterpret_cast<const Pds::Generic1D::ConfigV0*>(_cbuff);
      
      Xtc tc(Pds::TypeId(Pds::TypeId::Id_Generic1DConfig,0),_srv.client());
      tc.extent += c._sizeof();
      dg->insert(tc,_cbuff);
      if(_nerror) {
	printf("*** Found %d configuration errors\n", _nerror);
	dg->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
      }
    }
    break;
  default:
    break;
  }
  return dg;
}

void Manager::halt()
{
  _dev.stop();
}
