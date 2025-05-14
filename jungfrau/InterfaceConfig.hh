#ifndef Pds_Jungfrau_InterfaceConfig_hh
#define Pds_Jungfrau_InterfaceConfig_hh

#include <stdint.h>
#include <string>

namespace Pds {
  namespace Jungfrau {
    std::string getIpAddr(const std::string& hostname);
    std::string getDeviceName(const std::string& hostname);
    std::string getDetectorIp(const std::string& recvHost);
    std::string getReceiverMac(const std::string& recvHost);
    bool validateReceiverConfig(const std::string& recvHost, const std::string& recvMac, const std::string& detHost);
  }
}
#endif
