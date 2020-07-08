#ifndef Pds_IocControl_hh
#define Pds_IocControl_hh

#include "pds/utility/Appliance.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include <list>
#include <string>

namespace Pds {
  class IocHostCallback;
  class IocNode;
  class IocConnection;
  class IocOccurrence;

  class IocControl : public Appliance {
    friend class IocConnection;
  public:
    IocControl();
    IocControl(const char* offlinerc,
               const char* instrument,
               unsigned    station,
               const char*    expt_name,
               const char* controlrc,
               unsigned    pv_ignore=0);
    ~IocControl();
  public:
    /// Read the control configuration file.
    int read_controlrc(const char *controlrc);

    /// Write the global configuration to a new connection.
    void write_config(IocConnection *c, unsigned run, unsigned stream);

    /// Advertise the list of cameras available
    void host_rollcall(IocHostCallback*);

    /// Receive the list of cameras that will be recorded
    void set_partition(const std::list<DetInfo>&);

    const std::list<IocNode*>& selected() const { return _selected_nodes; }

  public:
    /// Act on a DAQ transition
    Transition* transitions(Transition*);
    InDatagram* events     (InDatagram* dg);

      //  private:
    void _report_error(const std::string&);
    void _report_data_warning(const std::string&);
    void _report_data_error(const std::string&, const unsigned, const unsigned);

  private:
    std::list<std::string> _offlinerc;   /// Logbook credentials
    std::string         _instrument;     /// Instrument
    unsigned            _station;        /// Instrument station
    std::string         _expt_name;      /// Experiment name
    unsigned            _pv_ignore;      /// Ignore setting for scalar PV recording
    int                 _recording;      /// Are we recording now?
    int                 _initialized;    /// Are we properly initialized?

    std::list<IocNode*> _nodes;          /// Available IOCs
    std::list<IocNode*> _selected_nodes; /// Selected IOCs

    char _trans[1024];

    IocOccurrence*      _occSender;      /// Handles sending occurances from Nodes
  };
};

#endif
