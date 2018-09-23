#ifndef Pds_Uxi_Manager_hh
#define Pds_Uxi_Manager_hh

namespace Pds {
  class Appliance;
  class CfgClientNfs;
  class Fsm;
  namespace Uxi {
    class Detector;
    class Server;
    class Manager {
    public:
      Manager (Detector&, Server&, CfgClientNfs&, unsigned);
      ~Manager();
    public:
      Appliance& appliance();
    private:
      Fsm& _fsm;
    };
  }
}

#endif
