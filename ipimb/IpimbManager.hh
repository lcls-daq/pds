#ifndef PDSIPIMBMANAGER_HH
#define PDSIPIMBMANAGER_HH

namespace Pds {

  class Fsm;
  class Appliance;
  class IpimbServer;
  class CfgClientNfs;
  class IpimBoard;

  class IpimbManager {
  public:
    IpimbManager(IpimbServer* server[], unsigned nServers, CfgClientNfs** cfg);
    Appliance& appliance();
  private:
    Fsm& _fsm;
    static const char* _calibPath;
    unsigned _nServers;
  };
}
#endif
