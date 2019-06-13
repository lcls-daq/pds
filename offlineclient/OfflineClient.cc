#include "OfflineClient.hh"
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include "pds/client/Action.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/TypeId.hh"
#include <stdio.h>

using namespace Pds;

//
// Partition Descriptor
//
PartitionDescriptor::PartitionDescriptor(const char *partition) :
  _partition_name(""),
  _instrument_name(""),
  _station_number(0u),
  _valid(false)
{
  unsigned int station = 0u;

  if (partition) {
    char instr[64];
    char *pColon;
    strncpy(instr, partition, sizeof(instr));
    pColon = index(instr, ':');
    if (pColon) {
      *pColon++ = '\0';
      if (isdigit(*pColon)) {
        errno = 0;
        station = strtoul(pColon, (char **)NULL, 10);
        if (!errno) {
          _valid = true;
        }
      }
    } else {
      _valid = true;
    }
    if (_valid) {
      _partition_name = partition;
      _instrument_name = instr;
      _station_number = station;
    }
  }
}

std::string PartitionDescriptor::GetPartitionName()
{
  return (_partition_name);
}

std::string PartitionDescriptor::GetInstrumentName()
{
  return (_instrument_name);
}

unsigned int PartitionDescriptor::GetStationNumber()
{
  return (_station_number);
}

bool PartitionDescriptor::valid()
{
  return (_valid);
}

//
// OfflineClient (current experiment name passed in)
//
//    /**
//    * Find experiment descriptor of the specified experiment if the one exists
//    *
//    * Return 'true' and initialize experiment descripton if there is
//    * such experiment. Return falase otherwise. In all other cases
//    * throw an exception.
//    */
//  virtual bool getOneExperiment (ExperDescr&        descr,
//                                 const std::string& instrument,
//                                 const std::string& experiment) throw (WrongParams,
//                                                                       DatabaseError) = 0 ;
//
//    make this call from anywhere in the DAQ system where you need to resolve the mapping:
//
//      {instrument_name,experiment_name} -> {experiment_id}
//
OfflineClient::OfflineClient(const char* path, PartitionDescriptor& pd, const char *experiment_name, bool verbose) :
    _pd(pd),
    _experiment_name (experiment_name),
    _client(NULL),
    _verbose(verbose),
    _isTesting(false)
{
    if (_verbose) {
      printf("entered %s: path='%s', instrument='%s:%u', experiment='%s'\n", __PRETTY_FUNCTION__,
             path, _pd.GetInstrumentName().c_str(), _pd.GetStationNumber(), _experiment_name.c_str());
    }
    _isTesting = (path == (char *)NULL) || (strcmp(path, "/dev/null") == 0); 
    if (_pd.valid()) {
      try {
         _client = WSLogbookClient::createWSLogbookClient(path, _experiment_name.c_str(), _pd.GetInstrumentName().c_str(), _pd.GetStationNumber(), _verbose);
         _info = _client->get_experiment_details();
      } catch(const std::runtime_error& e){
        fprintf(stderr, "Caught exception %s\n", e.what());
      }
    } else {
      fprintf(stderr, "%s: partition descriptor not valid\n", __PRETTY_FUNCTION__);
    }
    if (_verbose) {
      printf ("%s: experiment %s/%s \n", __PRETTY_FUNCTION__, _pd.GetInstrumentName().c_str(), _experiment_name.c_str());
    }
}

//
// OfflineClient (current experiment name retrieved from database)
//
OfflineClient::OfflineClient(const char* path, PartitionDescriptor& pd, bool verbose) :
    _pd(pd),
    _client(NULL),
    _verbose(verbose),
    _isTesting(false)
{
    if (_verbose) {
      printf("entered OfflineClient(path=%s, instr=%s, station=%u)\n", path, _pd.GetInstrumentName().c_str(), _pd.GetStationNumber());
    }
    _isTesting = (path == (char *)NULL) || (strcmp(path, "/dev/null") == 0); 
    if (_pd.valid()) {
      try {
         _client = WSLogbookClient::createWSLogbookClient(path, _experiment_name.c_str(), _pd.GetInstrumentName().c_str(), _pd.GetStationNumber(), _verbose);
         _info = _client->get_experiment_details();
         _experiment_name = _info.name;
      } catch(const std::runtime_error& e){
        fprintf(stderr, "Caught exception %s\n", e.what());
      }
      if (_verbose) {
        printf ("%s: instrument %s:%u experiment %s \n", __PRETTY_FUNCTION__, _pd.GetInstrumentName().c_str(), _pd.GetStationNumber(), _experiment_name.c_str());
      }
    } else {
      fprintf(stderr, "%s: partition descriptor not valid\n", __PRETTY_FUNCTION__);
    }
}

OfflineClient::~OfflineClient() {
    delete _client;
}

//
// BeginNewRun
//
// Begin new run: create a new run entry in the database.
//
// RETURNS: 0 if successful, otherwise -1
//
int OfflineClient::BeginNewRun(unsigned int *runNumber) {
  int returnVal = -1;  // default return is ERROR

  // sanity check
  if (_client && runNumber && !_experiment_name.empty()) {

    // in case of NULL database, do nothing
    if (_isTesting) {
      returnVal = 0;  // OK
    } else {
      printf("Starting a new run\n");
      try {
          unsigned int newRunNumber = _client->start_run();
          *runNumber = newRunNumber;
          printf("Started new run %d\n", newRunNumber);
          returnVal = 0; // OK
      } catch(const std::runtime_error& e){
          printf("Caught exception starting a new run %s\n", e.what());
          returnVal = -1; // ERROR
      }
    }
  }

  if (-1 == returnVal) {
    printf("%s returning error\n", __FUNCTION__);
  }

  return (returnVal);
}

int OfflineClient::EndCurrentRun() {
  int returnVal = -1;  // default return is ERROR

  // sanity check
  if (_client && !_experiment_name.empty()) {

    // in case of NULL database, do nothing
    if (_isTesting) {
      returnVal = 0;  // OK
    } else {
      printf("Ending the current run\n");
      try {
          _client->end_run();
          printf("Ended the current run\n");
          returnVal = 0; // OK
      } catch(const std::runtime_error& e){
          printf("Caught exception ending the current run%s\n", e.what());
          returnVal = -1; // ERROR
      }
    }
  }

  if (-1 == returnVal) {
    printf("%s returning error\n", __FUNCTION__);
  }

  return (returnVal);

}

int OfflineClient::reportOpenFile (std::string& dirpath, int run, int stream, int chunk, std::string& host, bool ffb) {
  int returnVal = -1;  // default return is ERROR

  // sanity check
  if (_client != NULL && !_experiment_name.empty()) {

    // in case of NULL database, report nothing
    if (_isTesting) {
      returnVal = 0;  // OK
    } else {
      try {
        _client->report_open_file(dirpath.c_str(), stream, chunk, host.c_str(), ffb);
        returnVal = 0; // OK
      } catch(const std::runtime_error& e){
          printf("Caught exception %s\n", e.what());
          returnVal = -1; // ERROR
      }
    }
  }

  if (-1 == returnVal) {
    fprintf(stderr, "Error reporting open file expt %s run %d stream %d chunk %d\n",
            _experiment_name.c_str(), run, stream, chunk);
  }

  return (returnVal);
}

int OfflineClient::reportDetectors (int run, std::vector<std::string>& names) {
  int returnVal = -1;  // default return is ERROR

  // sanity check
  if (_client != NULL && (names.size() > 0) && !_experiment_name.empty()) {
    // in case of NULL database, report nothing
    if (_isTesting) {
      returnVal = 0;  // OK
    } else {
      try {
        _client->reportDetectors(names);
        returnVal = 0; // OK
      } catch(const std::runtime_error& e){
          printf("Caught exception %s\n", e.what());
          returnVal = -1; // ERROR
      }
    }
  }
  return (returnVal);
}

int OfflineClient::reportTotals (int run, long events, long damaged, double gigabytes) {
  int returnVal = -1;  // default return is ERROR

  // sanity check
  if (_client != NULL && !_experiment_name.empty()) {
    // in case of NULL database, report nothing
    if (_isTesting) {
      returnVal = 0;  // OK
    } else {
      try {
        _client->reportTotals(events, damaged, gigabytes);
        returnVal = 0; // OK
      } catch(const std::runtime_error& e){
          printf("Caught exception %s\n", e.what());
          returnVal = -1; // ERROR
      }
    }
  }
  return (returnVal);
}


int OfflineClient::reportParams(int run, std::map<std::string, std::string> params) { 
  int returnVal = -1;  // default return is ERROR

  // sanity check
  if (_client != NULL && !_experiment_name.empty()) {
    // in case of NULL database, report nothing
    if (_isTesting) {
      returnVal = 0;  // OK
    } else {
      try {
        _client->add_run_params(params);
        returnVal = 0; // OK
      } catch(const std::runtime_error& e){
          printf("Caught exception %s\n", e.what());
          returnVal = -1; // ERROR
      }
    }
  }
  return (returnVal);
}

//
// GetExperimentName
//
std::string OfflineClient::GetExperimentName() {
  return _experiment_name;
}

//
// GetInstrumentName
//
std::string OfflineClient::GetInstrumentName() {
    return _pd.GetInstrumentName();
}

//
// GetStationNumber
//
unsigned int OfflineClient::GetStationNumber() {
    return _pd.GetStationNumber();
}
