#ifndef Pds_NsCam_Manager_hh
#define Pds_NsCam_Manager_hh

namespace Pds {
  class Appliance;
  class CfgClientNfs;
  class Fsm;
  namespace NsCam {
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
