#ifndef Pds_NsCam_Comm_hh
#define Pds_NsCam_Comm_hh

#include "pds/nscam/Types.hh"

#include <memory>
#include <string>

namespace Pds {
  namespace NsCam {
    class Comm {
    public:
      static void listDevices();
      static std::shared_ptr<Comm> create(const std::string& host, unsigned short port, CommType ctype);

      Comm(CommType ctype, const std::string& host, unsigned short port, unsigned long timeout);
      virtual ~Comm() = default;

      virtual uint32_t sendCmd(uint16_t cmd, uint16_t addr, uint32_t data=0) = 0;
      virtual bool openDevice() noexcept = 0;
      virtual bool closeDevice() noexcept = 0;
      virtual void info() const noexcept = 0;

      virtual CommType type() const;
      virtual std::string name() const;

    protected:
      CommType       ctype_;
      std::string    host_;
      unsigned short port_;      
      unsigned long  timeout_;
    };
  }
}

#endif
