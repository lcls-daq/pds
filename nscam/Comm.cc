#include "Comm.hh"
#include "Gige.hh"
#include "RS422.hh"

#include <sstream>

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

Comm::Comm(const std::string& host, unsigned short port, unsigned long timeout) :
  host_(host),
  port_(port),
  timeout_(timeout)
{}
