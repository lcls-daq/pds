#include "InterfaceConfig.hh"

#include <fstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ifaddrs.h>

std::string Pds::Jungfrau::getIpAddr(const std::string& hostname)
{
  hostent* entries = gethostbyname(hostname.c_str());
  if (entries) {
    return inet_ntoa(*(struct in_addr *)entries->h_addr_list[0]);
  } else {
    return "";
  }
}

std::string Pds::Jungfrau::getDeviceName(const std::string& hostname)
{
  std::string ipAddr = getIpAddr(hostname);
  if (ipAddr.empty()) {
    return "";
  }

  ifaddrs *ifp, *ifa;
  // use this to find the current interface in the ifaddrs
  in_addr_t s_addr = inet_addr(ipAddr.c_str());

  if (getifaddrs(&ifp) == -1) {
    perror("Error: call to getifaddrs failed:");
    return "";
  }

  for (ifa = ifp; ifa; ifa = ifa->ifa_next) {
    if (ifa->ifa_addr && ifa->ifa_addr->sa_family==AF_INET) {
      if (((sockaddr_in *) ifa->ifa_addr)->sin_addr.s_addr == s_addr) {
        return ifa->ifa_name;
      }
    }
  }

  return "";
}

std::string Pds::Jungfrau::getDetectorIp(const std::string& recvHost)
{
  std::string recvIp = getIpAddr(recvHost);
  if (recvIp.empty()) {
    fprintf(stderr, "Error: receiver hostname %s cannot be resolved to an ip address!\n", recvHost.c_str());
    return "";
  }

  size_t pos = recvIp.rfind('.');
  if (pos != std::string::npos) {
    int octet = (std::stoi(recvIp.substr(pos+1)) + 50) % 100;
    return recvIp.substr(0, pos+1) + std::to_string(octet);
  } else {
    fprintf(stderr, "Error: receiver ip addr %s is not a valid ip address!\n", recvIp.c_str());
    return "";
  }
}

std::string Pds::Jungfrau::getReceiverMac(const std::string& recvHost)
{
  std::string macAddr = "";
  std::string dev = getDeviceName(recvHost);
  std::string path = "/sys/class/net/" + dev + "/address";

  if (dev.empty()) {
    fprintf(stderr, "Error: unable to find device name for %s!\n", recvHost.c_str());
  } else {
    std::ifstream netf(path);
    if (netf.is_open()) {
      netf >> macAddr;
      netf.close();
    }
  }

  return macAddr;
}

bool Pds::Jungfrau::validateReceiverConfig(const std::string& recvHost, const std::string& recvMac, const std::string& detHost)
{
  std::string recvIp = getIpAddr(recvHost);
  std::string detIp = getIpAddr(detHost);
  std::string device = getDeviceName(recvHost);
  std::string actualMac = getReceiverMac(recvHost);

  if (recvIp == detIp) {
    fprintf(stderr, "Error: receiver ip addr %s and detector ip addr %s must be different!\n", recvIp.c_str(), detIp.c_str());
    return false;
  } else if (recvMac != actualMac) {
    fprintf(stderr, "Error: requested mac addr %s does not match the actual mac addr %s of interface %s with ip %s\n",
            recvMac.c_str(), actualMac.c_str(), device.c_str(), recvIp.c_str());
    return false;
  } else {
    return true;
  }
}

