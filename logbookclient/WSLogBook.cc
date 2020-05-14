#include "WSLogBook.hh"

#include "pds/service/PathTools.hh"

#include <strings.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <stdio.h>
#include <fstream>
#include <unistd.h>
#include <limits.h>

using namespace Pds;


WSLogbookClient* WSLogbookClient::createWSLogbookClient(
        const char* url,
        const char* uid,
        const char* passwd,
        bool useKerberos,
        const char* experiment_name,
        const char* instrument_name,
        const int station_num,
        bool verbose ) {
    if(url == NULL || strlen(url) <= 10) throw std::runtime_error("Need a server URL for the logbook cient to function correctly \n");
    std::string urlstr(url);
    if (*urlstr.rbegin() != '/') { urlstr.push_back('/'); }
    // Either we pass in the experiment name or instrument/station.
    bool exp_specified = experiment_name != NULL && strlen(experiment_name) >= 1;
    bool inssta_specified = instrument_name != NULL && strlen(instrument_name) >= 1 && station_num >= 0;
    if (!exp_specified && !inssta_specified) throw std::runtime_error("Please specify either the instrument/station or experiment name");
    bool wsauth = uid != NULL && strlen(uid) >= 1 && passwd != NULL;
    if (!wsauth && !useKerberos) throw std::runtime_error("Please specify at least one of userid+password or useKerberos for authentication");

    WSLogbookClient* ret = new WSLogbookClient(
        __mkstr__(url),
        __mkstr__(uid),
        __mkstr__(passwd),
        useKerberos,
        __mkstr__(experiment_name),
        __mkstr__(instrument_name),
        station_num,
        verbose
    );
    try {
        ret->__init__();
    } catch(...) {
        printf("Exception when initializing WSLogbookClient; cleaning up and rethrowing\n");
        delete ret;
        ret = NULL;
        throw;
    }
    return ret;
}

WSLogbookClient* WSLogbookClient::createWSLogbookClient( const std::string& configPath, const char* experiment_name, const char* instrument_name, const int station_num, bool verbose ) {
    std::map<std::string, std::string> configs;
    _parse_config_(configs, configPath, verbose);
    std::vector<std::string> config_keys; config_keys.push_back("logbook_endpoint"); config_keys.push_back("logbook_uid"); config_keys.push_back("logbook_pwd"); config_keys.push_back("logbook_use_kerberos");
    for(size_t i = 0; i < config_keys.size(); i++) {
        if(configs.find(std::string(config_keys[i])) == configs.end()) throw std::runtime_error("Config file " + configPath + " missing key '" + config_keys[i] + "' \n");
    }
    return createWSLogbookClient(
        configs[config_keys[0]].c_str(),
        configs[config_keys[1]].c_str(),
        configs[config_keys[2]].c_str(),
        configs[config_keys[3]] == std::string("true"),
        experiment_name,
        instrument_name,
        station_num,
        verbose
    );
}

void WSLogbookClient::__init__() {
    PathTools::initPythonPath(); // Add the DAQ release's PYTHONPATH to the environment
    Py_Initialize(); // This may need to be moved upstream to a main function.
    pModule = _CKADD_(PyImport_ImportModule("pds.logbookclient.lgbk_client"), "After importing and loading the lgbk_client module\n");
    PyObject* pModuleDict = _CK_(PyModule_GetDict(pModule));
    PyObject* pInitFunc = _CK_(PyDict_GetItemString(pModuleDict, "LogbookClient"));
    PyObject* pInitArgs = _CK_(Py_BuildValue("sssOO", _url.c_str(), _uid.c_str(), _passwd.c_str(), _useKerberos ? Py_True: Py_False , _verbose ? Py_True: Py_False));
    pClient = _CKADD_(PyObject_Call(pInitFunc, pInitArgs, NULL), "After creating the LogbookClient object instance");
    Py_XDECREF(pInitArgs);

    // Based on what was specified; determine the current experiment.
    if(_experimentName.empty()) {
        PyObject* pActiveExperimentName = _CK_(PyObject_CallMethod(pClient, "getActiveExperimentForInstrumentStation", "si", _instrumentName.c_str(), _stationNumber), "After getting the currently active experiment");
        PyObject* pExpNameUTF8 = _CK_(PyUnicode_AsUTF8String(pActiveExperimentName), "After decoding the experiment name as UTF-8");
        _experimentName = PyBytes_AsString(pExpNameUTF8);
        Py_XDECREF(pExpNameUTF8);
        Py_XDECREF(pActiveExperimentName);
    }

    PyObject* pExperimentDetails = _CK_(PyObject_CallMethod(pClient, "getExperimentDetails", "s", _experimentName.c_str()), "After getting the experiment details");
    _info.name = _dict_get_(pExperimentDetails, "name");
    _info.description = _dict_get_(pExperimentDetails, "description");
    _info.proposal_id = _dict_get_(pExperimentDetails, "proposal_id");
    _info.leader_account = _dict_get_(pExperimentDetails, "leader_account");
    _info.contact_info = _dict_get_(pExperimentDetails, "contact_info");
    _info.posix_gid = _dict_get_(pExperimentDetails, "posix_gid");
    _info.instrument_name = _dict_get_(pExperimentDetails, "instrument_name");
    Py_XDECREF(pExperimentDetails);
}

WSLogbookClient::~WSLogbookClient() {
    for (std::list<PyObject*>::reverse_iterator rit=_cleanupOnExit.rbegin(); rit!=_cleanupOnExit.rend(); ++rit) {
        Py_XDECREF(*rit);
    }
    Py_Finalize();
}

int WSLogbookClient::current_run() {
    _sem.take();
    PyObject* pRet = _CK_(PyObject_CallMethod(pClient, "getCurrentRun", "s", _experimentName.c_str()), "After getting the current run");
    if (pRet == Py_None) return -1; // This is an indication to the caller that a current run does not exist.
    int crun = PyLong_AsLong(pRet);
    Py_XDECREF(pRet);
    _sem.give();
    return crun;
}

int WSLogbookClient::start_run(const char* runtype) {
    _sem.take();
    PyObject* pRet = NULL;
    if(runtype == NULL) {
        pRet = _CK_(PyObject_CallMethod(pClient, "startRun", "s", _experimentName.c_str()), "After starting a new run");
    } else {
        pRet = _CK_(PyObject_CallMethod(pClient, "startRun", "ss", _experimentName.c_str(), runtype), "After starting a new run with run type");
    }
    int crun = PyLong_AsLong(pRet);
    Py_XDECREF(pRet);
    _sem.give();
    return crun;
}

int WSLogbookClient::end_run() {
    _sem.take();
    PyObject* pRet = _CK_(PyObject_CallMethod(pClient, "endRun", "s", _experimentName.c_str()), "After ending the current run");
    int crun = PyLong_AsLong(pRet);
    Py_XDECREF(pRet);
    _sem.give();
    return crun;
}

void WSLogbookClient::add_run_params(std::map<std::string, std::string>& name_value_pairs) {
    _sem.take();
    PyObject* pDict = PyDict_New();
    for (std::map<std::string, std::string>::iterator it = name_value_pairs.begin(); it != name_value_pairs.end(); ++it) {
        PyObject* val = PyUnicode_FromString(it->second.c_str());
        PyDict_SetItemString(pDict, it->first.c_str(), val);
        Py_XDECREF(val);
    }
    PyObject* pRet = _CK_(PyObject_CallMethod(pClient, "addRunParams", "sO", _experimentName.c_str(), pDict), "After adding run parameters");
    Py_XDECREF(pDict);
    Py_XDECREF(pRet);
    _sem.give();
    return;
}

void WSLogbookClient::add_run_params(std::map<std::string, VariantRunParam*>& name_value_pairs) {
    _sem.take();
    PyObject* pDict = PyDict_New();
    for (std::map<std::string, VariantRunParam*>::iterator it = name_value_pairs.begin(); it != name_value_pairs.end(); ++it) {
        PyDict_SetItemString(pDict, it->first.c_str(), it->second->pValObj);
    }
    PyObject* pRet = _CK_(PyObject_CallMethod(pClient, "addRunParams", "sO", _experimentName.c_str(), pDict), "After adding run parameters");
    Py_XDECREF(pDict);
    Py_XDECREF(pRet);
    for (std::map<std::string, VariantRunParam*>::iterator it = name_value_pairs.begin(); it != name_value_pairs.end(); ++it) {
        delete it->second; // We are responsible for freeing the VariantRunParam*
    }
    _sem.give();
    return;
}

void WSLogbookClient::reportDetectors (std::vector<std::string>& names) {
    std::map<std::string, VariantRunParam*> name_value_pairs;
    for(std::vector<std::string>::iterator it = names.begin(); it != names.end(); ++it) {
        name_value_pairs[std::string("DAQ Detectors/") + *it] = new VariantRunParam(true);
    }
    add_run_params(name_value_pairs);
}

void WSLogbookClient::reportTotals (long events, long damaged, double gigabytes) {
    std::map<std::string, VariantRunParam*> name_value_pairs;
    name_value_pairs[std::string("DAQ Detector Totals/Events")] = new VariantRunParam(events);
    name_value_pairs[std::string("DAQ Detector Totals/Damaged")] = new VariantRunParam(damaged);
    name_value_pairs[std::string("DAQ Detector Totals/Size")] = new VariantRunParam(gigabytes);
    add_run_params(name_value_pairs);
}

void WSLogbookClient::report_open_file (const char* path, int stream, int chunk, const char* hostName, bool ffb) {
    _sem.take();
    PyObject* pDict = PyDict_New();
    if(path != NULL) {
        // Pass in the path as an absolute path. The python code will generate the path
        PyObject* val = PyUnicode_FromString(path);
        PyDict_SetItemString(pDict, "absolute_path", val);
        Py_XDECREF(val);
    } else {
        _sem.give();
        throw std::runtime_error("Cannot register a file without the path.");
    }
    if(hostName != NULL) {
        PyObject* val = PyUnicode_FromString(hostName);
        PyDict_SetItemString(pDict, "hostname", val);
        Py_XDECREF(val);
    }

    PyObject* gen = PyLong_FromLong(1);
    PyDict_SetItemString(pDict, "gen", gen);
    Py_XDECREF(gen);

    PyObject* pRet = _CK_(PyObject_CallMethod(pClient, "registerFile", "sO", _experimentName.c_str(), pDict), "After registering file");
    Py_XDECREF(pDict);
    Py_XDECREF(pRet);
    _sem.give();
}

void WSLogbookClient::add_update_run_param_descriptions(std::map<std::string, std::string>& name_description_pairs) {
    _sem.take();
    PyObject* pDict = PyDict_New();
    for (std::map<std::string, std::string>::iterator it = name_description_pairs.begin(); it != name_description_pairs.end(); ++it) {
        PyObject* val = PyUnicode_FromString(it->second.c_str());
        PyDict_SetItemString(pDict, it->first.c_str(), val);
        Py_XDECREF(val);
    }
    PyObject* pRet = _CK_(PyObject_CallMethod(pClient, "addUpdateRunParamDescriptions", "sO", _experimentName.c_str(), pDict), "After adding/updating run parameter descriptions");
    Py_XDECREF(pDict);
    Py_XDECREF(pRet);
    _sem.give();
    return;
}

std::ostream& Pds::operator<<(std::ostream &strm, const ExperimentInfo &info) {
    return strm
        << "\nName: " << info.name
        << "\nDescription: " << info.description
        << "\nProposal id: " << info.proposal_id
        << "\nLeader Account: " << info.leader_account
        << "\nContact Info: " << info.contact_info
        << "\nPosix gid: " << info.posix_gid
        << "\nInstrument: " << info.instrument_name
        << "\n";
}


PyObject* WSLogbookClient::_CK_(PyObject* obj, const char* msg) {
    if (!obj) {
        PyErr_Print();
        _sem.give();
        throw std::runtime_error("Exception/error in the python runtime\n");
    }
    if(_verbose && ( msg != NULL) ) { printf(msg); }
    return obj;
}

PyObject* WSLogbookClient::_CKADD_(PyObject* obj, const char* msg) {
    _CK_(obj, msg);
    _cleanupOnExit.push_back(obj);
    return obj;
}

std::string WSLogbookClient::_dict_get_(PyObject* dict, const char* key) {
    PyObject* pVal = _CK_(PyDict_GetItemString(dict, key));
    PyObject* pValUTF8 = _CK_(PyUnicode_AsUTF8String(pVal));
    const char* cVal = PyBytes_AsString(pValUTF8);
    Py_XDECREF(pValUTF8);
    return cVal;
}

/**
  * Parse the configuration file specified by configPath
  * The config file is expected to have lines in the following format: <key>=[<value>]
  * Throws a std::runtime_error if errors are encountered
  */
void WSLogbookClient::_parse_config_(std::map<std::string, std::string>& configs, const std::string& configPath, bool verbose) {
    if( configPath.empty()) throw std::runtime_error("configPath seems to be empty; was the location parsed correctly?");
    if(verbose) { printf("Reading from config file '%s'\n", configPath.c_str()); }
    std::ifstream config_file( configPath.c_str());
    if( !config_file.good()) throw std::runtime_error( "Failed to open config file : '"+configPath+"'" );

    std::string line;
    while(config_file >> line) {
        if(verbose) { printf("Reading next line from config file '%s'\n", configPath.c_str()); }
        const size_t separator_pos = line.find( '=' );
        if( separator_pos != std::string::npos ) {
            const std::string key = line.substr( 0, separator_pos );
            const std::string value = line.substr( separator_pos + 1 );
            configs[key] = value;
            if(verbose) { printf("Read key %s %s from '%s'\n", key.c_str(), value.c_str(), configPath.c_str()); }
        } else {
            if(verbose) { printf("Next line missing = from config file '%s'\n", configPath.c_str()); }
        }
    }
}
