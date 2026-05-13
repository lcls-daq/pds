#include "Error.hh"

#include <string.h>

using namespace Pds::NsCam;

NsCamException::NsCamException(const std::string& error, const std::string& msg) :
  std::runtime_error(buildMsg(error, msg)),
  error_(error)
{}

NsCamException::~NsCamException() noexcept
{}

std::string NsCamException::error() const noexcept
{
  return error_;
}

std::string NsCamException::buildMsg(const std::string& error, const std::string& msg)
{
  return msg + ": " + error;
}

CommException::CommException(const std::string& code, const std::string& msg) :
  NsCamException(code, msg)
{}

CommException::~CommException() noexcept
{}

std::string CommException::code() const noexcept
{
  return error_;
}

GigeException::GigeException(ZESTETM1_STATUS status, const std::string& msg) :
  CommException(err2str(status), msg),
  status_(status)
{}

GigeException::GigeException(const std::string& host, const std::string& msg) :
  CommException(host, msg),
  status_(ZESTETM1_SUCCESS)
{}

GigeException::~GigeException()
{}

ZESTETM1_STATUS GigeException::status() const noexcept
{
  return status_;
}

std::string GigeException::err2str(ZESTETM1_STATUS status)
{
  char* msg;
  status = ZestETM1GetErrorMessage(status, &msg);
  if (status == ZESTETM1_SUCCESS) {
    return std::string(msg);
  } else {
    return "Unknown";
  }
}

RS422Exception::RS422Exception(int error, const std::string& msg) :
  CommException(strerror(error), msg)
{}

RS422Exception::RS422Exception(const std::string& error, const std::string& msg) :
  CommException(error, msg)
{}

RS422Exception::~RS422Exception()
{}

InvalidRegister::InvalidRegister(const std::string& regname) :
  NsCamException(regname, "Invalid register name")
{}

InvalidRegister::~InvalidRegister() noexcept
{}

std::string InvalidRegister::regname() const noexcept
{
  return error_;
}

ReadOnlyRegister::ReadOnlyRegister(const std::string& regname) :
  NsCamException(regname, "Cannot write to register")
{}

ReadOnlyRegister::~ReadOnlyRegister() noexcept
{}

std::string ReadOnlyRegister::regname() const noexcept
{
  return error_;
}

BoardError::BoardError(const std::string& error) :
  NsCamException(error, "Board internal error")
{}

BoardError::~BoardError() noexcept
{}
