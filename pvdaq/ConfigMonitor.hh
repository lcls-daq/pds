#ifndef Pds_PvDaq_ConfigMonitor_hh
#define Pds_PvDaq_ConfigMonitor_hh

#include "pds/epicstools/EpicsCA.hh"

namespace Pds {
  namespace PvDaq {
    class Server;

    class ConfigMonitor : public Pds_Epics::PVMonitorCb {
    public:
      ConfigMonitor(Server& server);
      ~ConfigMonitor();
    public:
      void updated();
    private:
      Server& _server;
    };

    class ConfigServer: public Pds_Epics::EpicsCA {
    public:
      ConfigServer(const char*, ConfigMonitor*);
      ~ConfigServer();
    public:
      const char* name() const;
      int  fetch      (void* copyTo, size_t len);
      int  fetch_str  (void* copyTo, size_t len);
      void update     ();
    private:
      char* _name;
    };
  }
}

#endif
