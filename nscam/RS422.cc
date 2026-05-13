#include "RS422.hh"
#include "Error.hh"

#include <iostream>
#include <iomanip>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <arpa/inet.h>

// Precomputed table for CRC-16/XMODEM (Polynomial 0x1021)
static const uint16_t crc16_xmodem_table[256] = {
  0x0000,
  0x1021,
  0x2042,
  0x3063,
  0x4084,
  0x50A5,
  0x60C6,
  0x70E7,
  0x8108,
  0x9129,
  0xA14A,
  0xB16B,
  0xC18C,
  0xD1AD,
  0xE1CE,
  0xF1EF,
  0x1231,
  0x0210,
  0x3273,
  0x2252,
  0x52B5,
  0x4294,
  0x72F7,
  0x62D6,
  0x9339,
  0x8318,
  0xB37B,
  0xA35A,
  0xD3BD,
  0xC39C,
  0xF3FF,
  0xE3DE,
  0x2462,
  0x3443,
  0x0420,
  0x1401,
  0x64E6,
  0x74C7,
  0x44A4,
  0x5485,
  0xA56A,
  0xB54B,
  0x8528,
  0x9509,
  0xE5EE,
  0xF5CF,
  0xC5AC,
  0xD58D,
  0x3653,
  0x2672,
  0x1611,
  0x0630,
  0x76D7,
  0x66F6,
  0x5695,
  0x46B4,
  0xB75B,
  0xA77A,
  0x9719,
  0x8738,
  0xF7DF,
  0xE7FE,
  0xD79D,
  0xC7BC,
  0x48C4,
  0x58E5,
  0x6886,
  0x78A7,
  0x0840,
  0x1861,
  0x2802,
  0x3823,
  0xC9CC,
  0xD9ED,
  0xE98E,
  0xF9AF,
  0x8948,
  0x9969,
  0xA90A,
  0xB92B,
  0x5AF5,
  0x4AD4,
  0x7AB7,
  0x6A96,
  0x1A71,
  0x0A50,
  0x3A33,
  0x2A12,
  0xDBFD,
  0xCBDC,
  0xFBBF,
  0xEB9E,
  0x9B79,
  0x8B58,
  0xBB3B,
  0xAB1A,
  0x6CA6,
  0x7C87,
  0x4CE4,
  0x5CC5,
  0x2C22,
  0x3C03,
  0x0C60,
  0x1C41,
  0xEDAE,
  0xFD8F,
  0xCDEC,
  0xDDCD,
  0xAD2A,
  0xBD0B,
  0x8D68,
  0x9D49,
  0x7E97,
  0x6EB6,
  0x5ED5,
  0x4EF4,
  0x3E13,
  0x2E32,
  0x1E51,
  0x0E70,
  0xFF9F,
  0xEFBE,
  0xDFDD,
  0xCFFC,
  0xBF1B,
  0xAF3A,
  0x9F59,
  0x8F78,
  0x9188,
  0x81A9,
  0xB1CA,
  0xA1EB,
  0xD10C,
  0xC12D,
  0xF14E,
  0xE16F,
  0x1080,
  0x00A1,
  0x30C2,
  0x20E3,
  0x5004,
  0x4025,
  0x7046,
  0x6067,
  0x83B9,
  0x9398,
  0xA3FB,
  0xB3DA,
  0xC33D,
  0xD31C,
  0xE37F,
  0xF35E,
  0x02B1,
  0x1290,
  0x22F3,
  0x32D2,
  0x4235,
  0x5214,
  0x6277,
  0x7256,
  0xB5EA,
  0xA5CB,
  0x95A8,
  0x8589,
  0xF56E,
  0xE54F,
  0xD52C,
  0xC50D,
  0x34E2,
  0x24C3,
  0x14A0,
  0x0481,
  0x7466,
  0x6447,
  0x5424,
  0x4405,
  0xA7DB,
  0xB7FA,
  0x8799,
  0x97B8,
  0xE75F,
  0xF77E,
  0xC71D,
  0xD73C,
  0x26D3,
  0x36F2,
  0x0691,
  0x16B0,
  0x6657,
  0x7676,
  0x4615,
  0x5634,
  0xD94C,
  0xC96D,
  0xF90E,
  0xE92F,
  0x99C8,
  0x89E9,
  0xB98A,
  0xA9AB,
  0x5844,
  0x4865,
  0x7806,
  0x6827,
  0x18C0,
  0x08E1,
  0x3882,
  0x28A3,
  0xCB7D,
  0xDB5C,
  0xEB3F,
  0xFB1E,
  0x8BF9,
  0x9BD8,
  0xABBB,
  0xBB9A,
  0x4A75,
  0x5A54,
  0x6A37,
  0x7A16,
  0x0AF1,
  0x1AD0,
  0x2AB3,
  0x3A92,
  0xFD2E,
  0xED0F,
  0xDD6C,
  0xCD4D,
  0xBDAA,
  0xAD8B,
  0x9DE8,
  0x8DC9,
  0x7C26,
  0x6C07,
  0x5C64,
  0x4C45,
  0x3CA2,
  0x2C83,
  0x1CE0,
  0x0CC1,
  0xEF1F,
  0xFF3E,
  0xCF5D,
  0xDF7C,
  0xAF9B,
  0xBFBA,
  0x8FD9,
  0x9FF8,
  0x6E17,
  0x7E36,
  0x4E55,
  0x5E74,
  0x2E93,
  0x3EB2,
  0x0ED1,
  0x1EF0,
};

static uint16_t crc16_xmodem_update(uint16_t crc, const uint8_t *data, size_t length) {
    for (size_t i = 0; i < length; ++i) {
        crc = (crc << 8) ^ crc16_xmodem_table[((crc >> 8) ^ data[i]) & 0xFF];
    }
    return crc;
}

#pragma pack(push)
#pragma pack(2)
class SerialPacket {
  public:
    SerialPacket(uint16_t cmd, uint16_t addr, uint32_t data=0);
    ~SerialPacket() = default;
    uint16_t command() const;
    uint16_t address() const;
    uint32_t data() const;
    uint16_t crc() const;
    bool check() const;

    void command(uint16_t cmd);
    void address(uint16_t addr);
    void data(uint32_t data);
  private:
    uint16_t calculateCrc() const;
    void updateCrc();
    uint16_t preamble_;
    uint16_t header_;
    uint32_t data_;
    uint16_t crc_;

    static const uint16_t addr_msk = 0xfff;
    static const uint16_t cmd_msk = 0xf;
    static const uint16_t cmd_shft = 12;
};
#pragma pack(pop)

SerialPacket::SerialPacket(uint16_t cmd, uint16_t addr, uint32_t data) :
  preamble_(0xaaaa),
  header_(htons(((cmd & cmd_msk)<<cmd_shft) | (addr & addr_msk))),
  data_(htonl(data)),
  crc_(0)
{
  updateCrc();
}

uint16_t SerialPacket::command() const
{
  return (ntohs(header_) >> cmd_shft) & cmd_msk;
}

uint16_t SerialPacket::address() const
{
  return ntohs(header_) & addr_msk;
}

uint32_t SerialPacket::data() const
{
  return ntohl(data_);
}

uint16_t SerialPacket::crc() const
{
  return ntohs(crc_);
}

bool SerialPacket::check() const
{
  return crc() == calculateCrc();
}

uint16_t SerialPacket::calculateCrc() const
{
  return crc16_xmodem_update(0, (uint8_t*) &header_, 6);
}

void SerialPacket::updateCrc()
{
  crc_ = htons(calculateCrc());
}

void SerialPacket::command(uint16_t cmd)
{
  header_ = htons(((cmd & cmd_msk)<<cmd_shft) | (ntohs(header_) & addr_msk));
  updateCrc();
}

void SerialPacket::address(uint16_t addr)
{
  header_ = htons((ntohs(header_) & (cmd_msk<<cmd_shft)) | (addr & addr_msk));
  updateCrc();
}

void SerialPacket::data(uint32_t data)
{
  data_ = htonl(data);
  updateCrc();
}

using namespace Pds::NsCam;

RS422::RS422(const std::string& device, speed_t baudrate, unsigned long timeout) :
  Comm(device, 0, timeout/100),
  fd_(-1),
  baudrate_(baudrate)
{
  connect();
}

RS422::~RS422()
{
  // close connection to camera
  closeDevice();
}

uint32_t RS422::sendCmd(uint16_t cmd, uint16_t addr, uint32_t data)
{
  SerialPacket packet(cmd, addr, data);

  ssize_t nbytes = 0;

  nbytes = write(fd_, &packet, sizeof(packet));
  if (nbytes < 0) {
    throw RS422Exception(errno, "Problem writing to device");
  }

  nbytes = read(fd_, &packet, sizeof(packet));
  if (nbytes < 0) {
    throw RS422Exception(errno, "Problem reading from device");
  } else if (nbytes != sizeof(packet)) {
    throw RS422Exception("Unexpected packet size", "Problem reading from device");
  } else if (!packet.check()) {
    throw RS422Exception("Corrupted packet", "Problem reading from device");
  }

  return packet.data();
}

bool RS422::openDevice() noexcept
{
  try {
    connect();
    return true;
  } catch (const RS422Exception& err) {
    return false;
  }
}

bool RS422::closeDevice() noexcept
{
  try {
    disconnect();
    return true;
  } catch (const RS422Exception& err) {
    return false;
  }
}

void RS422::info() const noexcept
{
  struct termios tty;
  // save the i/o formatting before changing...
  std::ios_base::fmtflags fmt(std::cout.flags());
  std::cout << "RS422 Interface Info:" << std::endl;
  std::cout << "=========================" << std::endl;
  std::cout << " Device:           " << host_ << std::endl;
  if (tcgetattr(fd_, &tty) != 0) {
    std::cout << " tcgetattr err:    " << strerror(errno) << std::endl;
  } else {
    std::cout << std::hex << std::setfill('0') << std::setw(8);
    std::cout << " c_iflag:          0x" << tty.c_iflag << std::endl;
    std::cout << " c_oflag:          0x" << tty.c_oflag << std::endl;
    std::cout << " c_cflag:          0x" << tty.c_cflag << std::endl;
    std::cout << " c_lflag:          0x" << tty.c_lflag << std::endl;
  }
  // restore i/o formatting
  std::cout.flags(fmt);
}

bool RS422::isConnected() const noexcept
{
  return fd_ >= 0;
}

void RS422::connect()
{
  fd_ = open(host_.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
  if (fd_ < 0) {
    throw RS422Exception(errno, "Problem opening device");
  }

   struct termios tty;

  // Get current serial port settings
  if (tcgetattr(fd_, &tty) != 0) {
    throw RS422Exception(errno, "Problem calling tcgetattr");
  }

  // Set baud rate
  cfsetospeed(&tty, baudrate_);
  cfsetispeed(&tty, baudrate_);

  // Configure 8N1 (8 data bits, no parity, 1 stop bit)
  tty.c_cflag |= PARENB;         // Enable parity
  tty.c_cflag |= PARODD;         // Set parity to ODD
  tty.c_cflag &= ~CSTOPB;        // 1 stop bit
  tty.c_cflag &= ~CSIZE;         // Clear data size bits
  tty.c_cflag |= CS8;            // 8 data bits
  tty.c_cflag &= ~CRTSCTS;       // No hardware flow control
  tty.c_cflag |= CREAD | CLOCAL; // Enable receiver, ignore modem control lines

  // Configure for raw input (non-canonical mode)
  tty.c_lflag &= ~ICANON;        // Disable canonical mode
  tty.c_lflag &= ~ECHO;          // Disable echo
  tty.c_lflag &= ~ECHOE;         // Disable erasure
  tty.c_lflag &= ~ECHONL;        // Disable new-line echo
  tty.c_lflag &= ~ISIG;          // Disable interpretation of INTR, QUIT and SUSP

  // Disable software flow control
  tty.c_iflag &= ~(IXON | IXOFF | IXANY);

  // Disable special handling of received bytes
  tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);

  // Disable output processing (raw output)
  tty.c_oflag &= ~OPOST;
  tty.c_oflag &= ~ONLCR;

  // Set read timeout (VTIME in deciseconds, VMIN in characters)
  tty.c_cc[VTIME] = timeout_;  // Wait for up to 1 second
  tty.c_cc[VMIN] = 0;          // Return as soon as any data is received

  // Apply the settings
  if (tcsetattr(fd_, TCSANOW, &tty) != 0) {
    throw RS422Exception(errno, "Problem calling tcsetattr");
  } 
}

void RS422::disconnect()
{
  if (close(fd_) < 0) {
    throw RS422Exception(errno, "Problem closing device");
  }
}
