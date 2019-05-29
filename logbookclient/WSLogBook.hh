
#ifndef WSLOGBOOK_HH
#define WSLOGBOOK_HH

#include <string>
#include <list>
#include <vector>
#include <map>
#include <iostream>
#include <stdexcept>

#include <Python.h>


namespace Pds {
     /**
      * Experiment descriptor; details about the experiment
      */
    struct _experiment_info {
        std::string name;
        std::string description;
        std::string proposal_id; // URAWI proposal ID; if any.
        std::string leader_account;
        std::string contact_info;
        std::string posix_gid;
        std::string instrument_name;
        friend std::ostream& operator<<(std::ostream &strm, const struct _experiment_info &info);
    };
    typedef _experiment_info ExperimentInfo;
    std::ostream& operator<<(std::ostream &strm, const ExperimentInfo &info);

  class WSLogbookClient;

  class VariantRunParam {
  public:
      VariantRunParam(bool val) { pValObj = _CK_(PyBool_FromLong(val)); }
      VariantRunParam(int val) { pValObj = _CK_(PyLong_FromLong(val)); }
      VariantRunParam(long val) { pValObj = _CK_(PyLong_FromLong(val)); }
      VariantRunParam(float val) { pValObj = _CK_(PyFloat_FromDouble(val)); }
      VariantRunParam(double val) { pValObj = _CK_(PyFloat_FromDouble(val)); }
      VariantRunParam(std::string val) { pValObj = _CK_(PyUnicode_FromString(val.c_str())); }
      virtual ~VariantRunParam() { Py_XDECREF(pValObj); }
      friend class WSLogbookClient;
  private:
      PyObject* pValObj;
      PyObject* _CK_(PyObject *obj) { if (!obj) { PyErr_Print(); throw std::runtime_error("Can't allocate variant\n"); }; return obj; }
  };

  class WSLogbookClient {
  public:
    static WSLogbookClient* createWSLogbookClient(
        const char* url, // The server endpoint URL; these are typically ws-kerb for Kerberos and ws-auth for the operator accounts. For example, https://<hostname>/ws-auth/lgbk/
        const char* uid, // The user id; could be the operator accounts.
        const char* passwd=NULL, // Password; if NULL, we use an empty string as the password.
        bool useKerberos=false, // Should we use Kerberos for authentication. User must have created a Kerberos token using kinit/aklog etc.
        const char* experiment_name=NULL, // If NULL, we expect instrument name/station and automatically set the current experiment to the current active experiment for the instrument/station combination.
        const char* instrument_name=NULL, // If NULL, we expect an experiment name
        const int station_num = 0, // Most instruments have only one station; station numbers start at 0.
        bool verbose=false ); // Any exception in the setup throws a std::runtime_error exception.

    // Create a WSLogbookClient using information from a config file. The configfile is expected to have these parameters as Name=Value lines
    // logbook_endpoint - The server endpoing URL
    // logbook_uid - Could be the
    // logbook_password
    // logbook_use_kerberos - Use the string "true" to use Kerberos for authentication.
    static WSLogbookClient* createWSLogbookClient(
        const std::string& configPath,
        const char* experiment_name=NULL, // If NULL, we expect instrument name/station and automatically set the current experiment to the current active experiment for the instrument/station combination.
        const char* instrument_name=NULL, // If NULL, we expect an experiment name
        const int station_num = 0, // Most instruments have only one station; station numbers start at 0.
        bool verbose=false ); // Any exception in the setup throws a std::runtime_error exception.


    virtual ~WSLogbookClient();

    std::string current_experiment() { return _info.name; }; // The current experiment for this WSLogbookClient object.
    const ExperimentInfo& get_experiment_details(){ return _info; }; // Details for the current experiment.

    // Run control. This is executed in the context of the current experiment.
    int start_run(const char* runtype=NULL); // Starts a new run; current_run is set to the new run number.
    int end_run(); // Ends the current run; note that current_run stays the same even after the end run.
    int current_run(); // The current run number; LCLS uses int's for run numbers. We return -1 if a current run is undefined; this can happen for new experiments before a run is started.

    // By default, all of these are executed in the context of the current experiment/current run.
    void add_run_params(std::map<std::string, std::string>& name_value_pairs); // Use this for registering strings; this is the bulk of the run params
    void add_run_params(std::map<std::string, VariantRunParam*>& name_value_pairs); // The variant version of the same. For simplicity, we expect a VariantRunParam* which this method will take the responsibility to free
    void reportDetectors (std::vector<std::string>& names); // Basically, add_run_params with boolean True's for the passed in names.
    void reportTotals (long events, long damaged, double gigabytes); // Basically, add_run_params with pre-defined names for some detector aggregates.

    // Register the file with the server. Note we flip the order of the argument around.
    void report_open_file (const char* path, int stream=-1, int chunk=-1, const char* hostName=NULL, bool ffb=false);

  private:
    WSLogbookClient(
        std::string url,
        std::string uid,
        std::string passwd,
        bool useKerberos,
        std::string experiment_name,
        std::string instrument_name,
        const int station_num,
        bool verbose)
    : _url(url), _uid(uid), _passwd(passwd), _useKerberos(useKerberos), _experimentName(experiment_name), _instrumentName(instrument_name), _stationNumber(station_num), _verbose(verbose) { }


    std::string _url;
    std::string _uid;
    std::string _passwd;
    bool _useKerberos;
    std::string _experimentName;
    std::string _instrumentName;
    int _stationNumber;
    bool _verbose;
    ExperimentInfo _info;

    // Boilerplate
    // All the python objects that need to be cleaned up on exit; these will be cleaned up in reverse order.
    std::list<PyObject*> _cleanupOnExit;
    // Other PyObjects; these are still cleaned up as part of the _cleanupOnExit framework.
    PyObject *pModule;
    PyObject *pClient;
    void __init__();
    static std::string __mkstr__(const char* s) { return s == NULL ? std::string() : std::string(s); }
    PyObject* _CK_(PyObject* obj, const char* msg=NULL);
    PyObject* _CKADD_(PyObject* obj, const char* msg=NULL);
    std::string _dict_get_(PyObject* dict, const char* key);
    static void _parse_config_(std::map<std::string, std::string>& configs, const std::string& configPath, bool verbose);
  };

}

#endif
