#include "pds/logbookclient/WSLogBook.hh"

#include <getopt.h>
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace Pds;

int main( int argc, char** argv ) {
  int c;
  char* config = NULL;
  char* url = NULL;
  char* uid = NULL;
  char* passwd = NULL;
  bool useKerberos = false;
  char* experiment_name = NULL;
  char* instrument_name = NULL;
  int station_num = 0;
  bool verbose = false;

  while (1) {
      static struct option long_options[] =
        {
          {"verbose",      no_argument,        0, 'v'},
          {"config",       required_argument,  0, 'c'},
          {"url",          required_argument,  0, 'l'},
          {"userid",       required_argument,  0, 'u'},
          {"password",     required_argument,  0, 'p'},
          {"kerberos",     no_argument,        0, 'k'},
          {"instrument",   required_argument,  0, 'i'},
          {"station",      required_argument,  0, 's'},
          {"experiment",   required_argument,  0, 'e'},
          {0, 0, 0, 0}
        };
      /* getopt_long stores the option index here. */
      int option_index = 0;

      c = getopt_long (argc, argv, "vl:u:p:ki:s:e:", long_options, &option_index);
      if (c == -1) break;
      switch (c)
      {
        case 'v': verbose = true; break;
        case 'c': config = optarg; break;
        case 'l': url = optarg; break;
        case 'u': uid = optarg; break;
        case 'p': passwd = optarg; break;
        case 'k': useKerberos = true; break;
        case 'i': instrument_name = optarg; break;
        case 's': station_num = atoi(optarg); break;
        case 'e': experiment_name = optarg; break;

        case '?': break; /* getopt_long already printed an error message. */
        default:  abort ();
        }
    }

    try {
        WSLogbookClient* client = NULL;
        if(config != NULL) {
            client = WSLogbookClient::createWSLogbookClient(std::string(config), experiment_name, instrument_name, station_num, verbose);
        } else {
            client = WSLogbookClient::createWSLogbookClient(url, uid, passwd, useKerberos, experiment_name, instrument_name, station_num, verbose);
        }
        std::cout << "Current experiment is " << client->current_experiment() << std::endl;
        std::cout << client->get_experiment_details() << std::endl;
        std::cout << "Current run: " << client->current_run() << std::endl;
        std::cout << "Starting a new run: " << client->start_run() << std::endl;
        std::cout << "Now the current run is: " << client->current_run() << std::endl;
        std::map<std::string, std::string> name_value_pairs;
        for(int p = 0; p < 1000; p++) {
            std::stringstream skey, sval;
            skey << "Key:" << p;
            sval << "Value:" << p;
            name_value_pairs[skey.str()] =  sval.str();
        }
        client->add_run_params(name_value_pairs);
        std::cout << "After adding run parameters" << std::endl;
        const char* dnames[] = {"BldEb-0|NoDevice-0", "EpicsArch-0|NoDevice-0", "NoDetector-0|Evr-0", "NoDetector-0|Evr-1", "XppEndstation-0|USDUSB-0", "XppSb3Pim-1|Tm6740-1"};
        std::vector<std::string> detectors(dnames, dnames + sizeof(dnames) / sizeof(char*));
        client->reportDetectors(detectors);
        std::cout << "After reporting detectors" << std::endl;
        client->reportTotals(12345, 1234, 314.15926);
        std::cout << "After reporting detector totals" << std::endl;
        char fname[255];
        sprintf(fname,"%s/%s/xtc/%s-r%04d-s%02d-c%02d.xtc", "XPP", client->current_experiment().c_str(), client->current_experiment().c_str(), 0, 0, 0);
        client->report_open_file(fname, 0, 0, "sample_hostname", true);
        std::cout << "After registering file" << std::endl;
        std::cout << "Starting a new run of type TEST: " << client->start_run("TEST") << std::endl;
        std::cout << "Now the current run is: " << client->current_run() << std::endl;
        std::cout << "Ending the current run: " << client->end_run() << std::endl;
        std::cout << "Now the current run should be the same: " << client->current_run() << std::endl;

        delete client;
    } catch(const std::runtime_error& e){
        printf("Caught exception %s\n", e.what());
    }
}
