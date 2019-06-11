
#ifndef OFFLINECLIENT_HH
#define OFFLINECLIENT_HH

#include "pds/logbookclient/WSLogBook.hh"

#include <vector>

#define OFFLINECLIENT_DEFAULT_EXPNAME ""
#define OFFLINECLIENT_DEFAULT_EXPNUM  0
#define OFFLINECLIENT_DEFAULT_STATION 0u
#define OFFLINECLIENT_MAX_PARMS       1000

namespace Pds {

  class PartitionDescriptor {
  public:
    PartitionDescriptor(const char *name);
    std::string GetPartitionName();
    std::string GetInstrumentName();
    unsigned int GetStationNumber();
    bool valid();

  private:
    std::string _partition_name;
    std::string _instrument_name;
    unsigned int _station_number;
    bool _valid;
  };

  class OfflineClient {
  public:
    OfflineClient(const char* path, PartitionDescriptor& pd, const char *experiment_name, bool verbose=true);
    OfflineClient(const char* path, PartitionDescriptor& pd, bool verbose=true);
    virtual ~OfflineClient();
    int BeginNewRun(unsigned int *runNumber);
    int EndCurrentRun();
    int reportOpenFile (std::string& dirpath, int run, int stream, int chunk, std::string& host, bool ffb=false);
    int reportParams (int run, std::map<std::string, std::string> params);
    int reportDetectors (int run, std::vector<std::string>& names);
    int reportTotals (int run, long events, long damaged, double gigabytes);

    std::string GetExperimentName();
    std::string GetInstrumentName();
    unsigned int GetStationNumber();

  private:
    PartitionDescriptor _pd;
    std::string _experiment_name;
    ExperimentInfo _info;
    WSLogbookClient* _client;
    bool _verbose;
    bool _isTesting;
  };

}

#endif
