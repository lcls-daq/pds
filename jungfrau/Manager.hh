#ifndef Pds_Jungfrau_Manager_hh
#define Pds_Jungfrau_Manager_hh

namespace Pds {
  class Appliance;
  class Fsm;
  namespace Jungfrau {
    class Detector;
    class Server;
    class DetIdLookup;
    class Manager {
    public:
      Manager (Detector&, Server&, DetIdLookup&);
      ~Manager();
    public:
      Appliance& appliance();
    private:
      Fsm& _fsm;
    };
  }
}

#endif
