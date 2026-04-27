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
      enum Type { NUMERIC = 0, STR = 1, LONGSTR = 2, NONE = 3 };
      ConfigServer(const char* name, ConfigMonitor* cfgmon);
      ConfigServer(const char* name, ConfigMonitor* cfgmon, bool is_enum);
      ConfigServer(const char* name, ConfigMonitor* cfgmon, bool is_enum,
                   void* copyTo, size_t len);
      ConfigServer(const char* name, ConfigMonitor* cfgmon, bool is_enum,
                   void* copyTo, size_t len, Type type);

      ~ConfigServer();
    public:
      const char* name() const;
      int  fetch      (void* copyTo, size_t len);
      int  fetch_str  (void* copyTo, size_t len);
      int  fetch      ();
      bool check_fetch();
      void update     ();
    private:
      char*  _name;
      void*  _copyTo;
      size_t _len;
      Type   _type;
    };
  }
}

#endif
