#ifndef Pds_QuadAdc_Manager_hh
#define Pds_QuadAdc_Manager_hh

#include "pds/utility/Appliance.hh"
//#include "pds/config/TriggerConfigType.hh"
#include "pds/config/CfgClientNfs.hh"
#include "pds/utility/Appliance.hh"
#include "pds/service/GenericPool.hh"
#include "pds/service/Routine.hh"
#include "pds/service/Task.hh"
#include "pds/service/Timer.hh"
//#include "pds/quadadc/StatsTimer.hh"
//#include "pds/quadadc/EpicsCfg.hh"
//#include "pds/quadadc/QABase.hh"
#include "pds/config/QuadAdcConfigType.hh"

namespace Pds {
    class CfgClientNfs;
  namespace HSD {
    class Module;
  }
  namespace QuadAdc {
    class Server;
    //class QABase;
    
    class Manager : public Appliance {
    public:
      Manager (Pds::HSD::Module&, Server&, CfgClientNfs&, int);
      ~Manager();
    public:	
      Transition* transitions(Transition*);
      InDatagram* events     (InDatagram*);
      void        halt       ();
    private:
      HSD::Module&           _dev;
      Server&           _srv;
      CfgClientNfs&     _cfg;
      // QABase&           _qab;
      char*	        _cbuff;
      int                  _testpattern;
      GenericPool       _occPool;
      unsigned          _nerror;
      QuadAdcConfigType    _config;
      Xtc                  _cfgtc;

     
    };
  }
}




#endif
