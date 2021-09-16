#ifndef Pds_Vimba_Manager_hh
#define Pds_Vimba_Manager_hh

namespace Pds {
  class Appliance;
  class Fsm;
  namespace Vimba {
    class FrameBuffer;
    class Server;
    class ConfigCache;
    class Manager {
    public:
      Manager (FrameBuffer&, Server&, ConfigCache&);
      ~Manager();
    public:
      Appliance& appliance();
    private:
      Fsm& _fsm;
    };
  }
}

#endif
