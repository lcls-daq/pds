#ifndef PDSEVRSIMMANAGER_HH
#define PDSEVRSIMMANAGER_HH

#include "EvgrOpcode.hh"

namespace Pds {

  class Appliance;
  class CfgClientNfs;
  class EvrFifoServer;
  class EvrTimer;
  class Fsm;
  class Server;
  class SimEvr;

  class EvrSimManager {
  public:  
    EvrSimManager(CfgClientNfs& cfg);
    ~EvrSimManager();

    // SIGINT handler
    static void sigintHandler(int);
    static void randomize_nodes(bool);
    
    Appliance&  appliance();
    Server&     server();
  private: 
    Fsm&           _fsm;
    EvrFifoServer* _server;
    EvrTimer*      _done;
    SimEvr&        _er;
    unsigned*      _events;
  };
}

#endif
