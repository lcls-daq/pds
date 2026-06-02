#ifndef Pds_NsCam_Error_hh
#define Pds_NsCam_Error_hh

#include <string>
#include <stdexcept>

#include "ZestETM1/ZestETM1.h"

namespace Pds {
  namespace NsCam {
    class NsCamException : public std::runtime_error {
    public:
      NsCamException(const std::string& error, const std::string& msg);
      virtual ~NsCamException() noexcept;
      std::string error() const noexcept;
    protected:
      static std::string buildMsg(const std::string& error, const std::string& msg);
      std::string error_;
    };

    class CommException : public NsCamException {
    public:
      CommException(const std::string& code, const std::string& msg);
      virtual ~CommException() noexcept;
      std::string code() const noexcept;
    };

    class GigeException : public CommException {
    public:
      GigeException(ZESTETM1_STATUS status, const std::string& msg);
      GigeException(const std::string& host, const std::string& msg);
      virtual ~GigeException() noexcept;
      ZESTETM1_STATUS status() const noexcept;
      static std::string err2str(ZESTETM1_STATUS status);
    private:
      ZESTETM1_STATUS status_;
    };

    class RS422Exception : public CommException {
    public:
      RS422Exception(int error, const std::string& msg);
      RS422Exception(const std::string& error, const std::string& msg);
      virtual ~RS422Exception() noexcept;
    };

    class InvalidRegister : public NsCamException {
    public:
      InvalidRegister(const std::string& regname);
      ~InvalidRegister() noexcept;
      std::string regname() const noexcept;
    };

    class ReadOnlyRegister : public NsCamException {
    public:
      ReadOnlyRegister(const std::string& regname);
      ~ReadOnlyRegister() noexcept;
      std::string regname() const noexcept;
    };

    class BoardError : public NsCamException {
    public:
      BoardError(const std::string& error);
      ~BoardError() noexcept;
    };

    class PotError : public NsCamException {
    public:
      PotError(const std::string& potname, const std::string& error);
      ~PotError() noexcept;
    };
  }
}

#endif
