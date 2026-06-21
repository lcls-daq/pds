#include "Comm.hh"
#include "Gige.hh"
#include "RS422.hh"

#include <sstream>
#include <cstring>
#include <arpa/inet.h>

using namespace Pds::NsCam;

void Comm::listDevices()
{
  Gige::listDevices();
}

std::shared_ptr<Comm> Comm::create(const std::string& host, unsigned short port, CommType ctype)
{
  switch (ctype) {
  case CommType::RS422:
    return std::make_shared<RS422>(host);
  case CommType::GIGE:
    return std::make_shared<Gige>(host, port);
  default:
    return std::shared_ptr<Comm>{};
  }
}

Comm::Comm(CommType ctype, const std::string& host, unsigned short port, unsigned long timeout) :
  ctype_(ctype),
  host_(host),
  port_(port),
  timeout_(timeout),
  buffer_size_(0),
  buffer_(nullptr)
{}

CommType Comm::type() const
{
  return ctype_;
}

std::string Comm::name() const
{
  return toString(ctype_);
}

std::string Comm::host() const
{
  return host_;
}

unsigned short Comm::port() const
{
  return port_;
}

std::unique_ptr<uint8_t[]> Comm::readDataAsUInt8(uint16_t addr, uint32_t data, size_t len)
{
  size_t pos = readData(addr, data, len);
  std::unique_ptr<uint8_t[]> conv(new uint8_t[len]);
  std::memcpy(conv.get(), buffer_.get() + pos, len);
  return conv;
}

std::unique_ptr<uint16_t[]> Comm::readDataAsUInt16(uint16_t addr, uint32_t data, size_t len)
{
  size_t pos = readData(addr, data, len * sizeof(uint16_t));
  std::unique_ptr<uint16_t[]> conv(new uint16_t[len]);
  uint16_t* raw = reinterpret_cast<uint16_t*>(buffer_.get() + pos);
  for (size_t idx=0; idx<len; idx++) {
    conv[idx] = ntohs(raw[idx]);
  }
  return conv;
}

std::unique_ptr<uint32_t[]> Comm::readDataAsUInt32(uint16_t addr, uint32_t data, size_t len)
{
  size_t pos = readData(addr, data, len * sizeof(uint32_t));
  std::unique_ptr<uint32_t[]> conv(new uint32_t[len]);
  uint32_t* raw = reinterpret_cast<uint32_t*>(buffer_.get() + pos);
  for (size_t idx=0; idx<len; idx++) {
    conv[idx] = ntohl(raw[idx]);
  }
  return conv;
}

bool Comm::prepBuffer(size_t size)
{
  if (size > buffer_size_) {
    buffer_.reset(new uint8_t[size]);
    buffer_size_ = size;
  }
  return buffer_ != nullptr;
}
