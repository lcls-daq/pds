#include "Gige.hh"
#include "Error.hh"

#include <iostream>
#include <sstream>
#include <vector>
#include <arpa/inet.h>
#include <unistd.h>

#pragma pack(push)
#pragma pack(2)
class GigePacket {
  public:
    GigePacket(uint16_t cmd, uint16_t addr, uint32_t data=0);
    ~GigePacket() = default;
    uint16_t command() const;
    uint16_t address() const;
    uint32_t data() const;

    void command(uint16_t cmd);
    void address(uint16_t addr);
    void data(uint32_t data);
  private:
    uint16_t preamble_;
    uint16_t header_;
    uint32_t data_;

    static const uint16_t addr_msk = 0xfff;
    static const uint16_t cmd_msk = 0xf;
    static const uint16_t cmd_shft = 12;
};
#pragma pack(pop)

GigePacket::GigePacket(uint16_t cmd, uint16_t addr, uint32_t data) :
  preamble_(0xaaaa),
  header_(htons(((cmd & cmd_msk)<<cmd_shft) | (addr & addr_msk))),
  data_(htonl(data))
{}

uint16_t GigePacket::command() const
{
  return (ntohs(header_) >> cmd_shft) & cmd_msk;
}

uint16_t GigePacket::address() const
{
  return ntohs(header_) & addr_msk;
}

uint32_t GigePacket::data() const
{
  return ntohl(data_);
}

void GigePacket::command(uint16_t cmd)
{
  header_ = htons(((cmd & cmd_msk)<<cmd_shft) | (ntohs(header_) & addr_msk));
}

void GigePacket::address(uint16_t addr)
{
  header_ = htons((ntohs(header_) & (cmd_msk<<cmd_shft)) | (addr & addr_msk));
}

void GigePacket::data(uint32_t data)
{
  data_ = htonl(data);
}

constexpr size_t DataHeaderSize = 2 * sizeof(GigePacket);

static std::string ip2str(const unsigned char (&ipaddr)[4])
{
  std::ostringstream ipsstr;
  ipsstr << int(ipaddr[0]) << ".";
  ipsstr << int(ipaddr[1]) << ".";
  ipsstr << int(ipaddr[2]) << ".";
  ipsstr << int(ipaddr[3]);

  return ipsstr.str();
}

static std::string mac2str(const unsigned char (&macaddr)[6])
{
  std::ostringstream macstr;
  for (int m=0; m<6; m++) {
    if (m==0) {
      macstr << std::hex << int(macaddr[m]);
    } else {
      macstr << ":" << int(macaddr[m]);
    }
  }

  return macstr.str();
}

using namespace Pds::NsCam;

void Gige::listDevices(unsigned long wait)
{
  ZESTETM1_STATUS status = ZestETM1Init();
  
  if (status == ZESTETM1_SUCCESS) {
    unsigned long num_cards;
    ZESTETM1_CARD_INFO* info;
    
    status = ZestETM1CountCards(&num_cards, &info, wait);
    if (status == ZESTETM1_SUCCESS) {
      std::cout << "Found " << num_cards << " devices:" << std::endl;
      for (unsigned long n=0; n<num_cards; n++) {
        std::cout << "====================" << std::endl;
        std::cout << " Ip Address:       " << ip2str(info[n].IPAddr) << std::endl;
        std::cout << " Control Port:     " << info[n].ControlPort << std::endl;
        std::cout << " Timeout:          " << info[n].Timeout << std::endl;
        std::cout << " HTTP Port:        " << info[n].HTTPPort << std::endl;
        std::cout << " MAC Address:      " << mac2str(info[n].MACAddr) << std::endl;
        std::cout << " Netmask:          " << ip2str(info[n].SubNet) << std::endl;
        std::cout << " Gateway:          " << ip2str(info[n].Gateway) << std::endl;
        std::cout << " Serial Number:    " << info[n].SerialNumber << std::endl;
      }

      // free the memory for info
      ZestETM1FreeCards(info);
    }
  } else {
    std::cout << "Problem calling ZestETM1Init: " << GigeException::err2str(status) << std::endl;
  }
}

Gige::Gige(const std::string& host, unsigned short port, unsigned long timeout, unsigned long wait) :
  Comm(CommType::GIGE, host, port, timeout),
  wait_(wait),
  conn_(nullptr)
{
  ZESTETM1_STATUS status = ZestETM1Init();
  if (status != ZESTETM1_SUCCESS) {
    throw GigeException(status, "Problem calling ZestETM1Init");
  }

  // if no gige was found this will return false
  if (!discovery()) {
    throw GigeException(host_, "No camera found with requested hostname"); 
  }

  status = connect();
  if (status != ZESTETM1_SUCCESS) {
    throw GigeException(status, "Problem calling ZestETM1OpenConnection");
  }
}

Gige::~Gige()
{
  // close connection to camera
  closeDevice();
  if (closeDevice()) {
    // close the zest context
    ZestETM1Close();
  }
}

bool Gige::openDevice() noexcept
{
  return connect() == ZESTETM1_SUCCESS;
}

bool Gige::closeDevice() noexcept
{
  return disconnect() == ZESTETM1_SUCCESS;
}

void Gige::info() const noexcept
{
  std::cout << "Gige Interface Info:" << std::endl; 
  std::cout << "=========================" << std::endl;
  std::cout << " Ip Address:       " << ip2str(info_.IPAddr) << std::endl;
  std::cout << " Control Port:     " << info_.ControlPort << std::endl;
  std::cout << " Timeout:          " << info_.Timeout << std::endl;
  std::cout << " HTTP Port:        " << info_.HTTPPort << std::endl;
  std::cout << " MAC Address:      " << mac2str(info_.MACAddr) << std::endl;
  std::cout << " Netmask:          " << ip2str(info_.SubNet) << std::endl;
  std::cout << " Gateway:          " << ip2str(info_.Gateway) << std::endl;
  std::cout << " Serial Number:    " << info_.SerialNumber << std::endl;
  std::cout << " Firmware Version: " << info_.FirmwareVersion << std::endl;
  std::cout << " Hardware Version: " << info_.HardwareVersion << std::endl;
  std::cout << std::endl;
}

void Gige::reconnect()
{
  ZESTETM1_STATUS status = disconnect();
  if (status !=  ZESTETM1_SUCCESS) {
    throw GigeException(status, "Problem calling ZestETM1CloseConnection");
  }

  // discovery needs to be redone or connect will not work
  if (!discovery()) {
    throw GigeException(host_, "No camera found with requested hostname");
  }

  status = connect();
  if (status !=  ZESTETM1_SUCCESS)  {
    throw GigeException(status, "Problem calling ZestETM1OpenConnection");
  }
}

size_t Gige::readData(uint16_t addr, uint32_t data, size_t payloadSize)
{
  GigePacket packet(0, addr, data);

  unsigned long nbytes = 0;
  ZESTETM1_STATUS status = writeSerial(&packet, sizeof(packet), &nbytes);
  if (status != ZESTETM1_SUCCESS) {
    throw GigeException(status, "Problem calling ZestETM1WriteData");
  } else if (nbytes != sizeof(packet)) {
    throw GigeException("Unexpected packet size", "Problem calling ZestETM1WriteData");
  }

  if (!prepBuffer(payloadSize + DataHeaderSize)) {
    throw GigeException("Unable to allocate buffer", "Problem calling prepBuffer");
  }

  status = readSerial(buffer_.get(), buffer_size_, &nbytes);
  if (status != ZESTETM1_SUCCESS) {
    throw GigeException(status, "Problem calling ZestETM1ReadData");
  } else if (nbytes != buffer_size_) {
    throw GigeException("Unexpected packet size", "Problem calling ZestETM1ReadData");
  }

  const GigePacket* payloadPacket = reinterpret_cast<const GigePacket*>(buffer_.get()) + 1;
  if (payloadPacket->data() != payloadSize) {
    throw GigeException("Unexpected packet size metadata", "Problem calling ZestETM1ReadData");
  }

  return DataHeaderSize;
}

uint32_t Gige::sendCmd(uint16_t cmd, uint16_t addr, uint32_t data)
{
  GigePacket packet(cmd, addr, data);

  unsigned long nbytes = 0;
  ZESTETM1_STATUS status = writeSerial(&packet, sizeof(packet), &nbytes);
  if (status != ZESTETM1_SUCCESS) {
    throw GigeException(status, "Problem calling ZestETM1WriteData");
  } else if (nbytes != sizeof(packet)) {
    throw GigeException("Unexpected packet size", "Problem calling ZestETM1WriteData");
  }

  status = readSerial(&packet, sizeof(packet), &nbytes);
  if (status != ZESTETM1_SUCCESS) {
    throw GigeException(status, "Problem calling ZestETM1ReadData");
  } else if (nbytes != sizeof(packet)) {
    throw GigeException("Unexpected packet size", "Problem calling ZestETM1ReadData");
  }

  return packet.data();
}

bool Gige::discovery()
{
  bool found = false;
  unsigned long attempt = 0;
  do {
    unsigned long num_cards = 0;
    ZESTETM1_CARD_INFO* info = NULL;
    ZESTETM1_STATUS status = ZestETM1CountCards(&num_cards, &info, 1000);
    if (status != ZESTETM1_SUCCESS) {
      throw GigeException(status, "Problem calling ZestETM1CountCards");
    }

    // look for cards with requested ip
    for (unsigned long n=0; n<num_cards; n++) {
      if (ip2str(info[n].IPAddr) == host_) {
        info_ = info[n];
        found = true;
        break;
      }
    }

    // free the memory for info structs from ZestETM1CountCards
    ZestETM1FreeCards(info);

    // found the camera so break
    if (found) {
      break;
    }

    attempt++;
  } while (attempt < wait_);

  return found;
}

bool Gige::isConnected() const noexcept
{
  return conn_ != nullptr;
}

ZESTETM1_STATUS Gige::connect() noexcept
{
  ZESTETM1_STATUS status = ZESTETM1_SUCCESS;
  if (!isConnected()) {
    status = ZestETM1OpenConnection(&info_, ZESTETM1_TYPE_TCP, port_, 0, &conn_);
  }
  return status;
}

ZESTETM1_STATUS Gige::disconnect() noexcept
{
  ZESTETM1_STATUS status = ZESTETM1_SUCCESS;
  if (isConnected()) {
    status = ZestETM1CloseConnection(conn_);
    conn_ = nullptr;
  }
  return status;
}

ZESTETM1_STATUS Gige::readSerial(void* buffer, unsigned long len, unsigned long* nbytes) noexcept
{
  return ZestETM1ReadData(conn_, buffer, len, nbytes, timeout_);
}

ZESTETM1_STATUS Gige::writeSerial(void* buffer, unsigned long len, unsigned long* nbytes) noexcept
{
  return ZestETM1WriteData(conn_, buffer, len, nbytes, timeout_);
}
