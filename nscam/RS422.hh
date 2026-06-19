#ifndef Pds_NsCam_RS422_hh
#define Pds_NsCam_RS422_hh

#include "pds/nscam/Comm.hh"

#include <termios.h>

namespace Pds {
  namespace NsCam {
    class RS422 : public Comm {
    public:
      RS422(const std::string& device, speed_t baudrate=B921600, unsigned long timeout=10000);
      virtual ~RS422() noexcept;

      virtual uint32_t sendCmd(uint16_t cmd, uint16_t addr, uint32_t data=0) override;
      virtual bool openDevice() noexcept override;
      virtual bool closeDevice() noexcept override;
      virtual void info() const noexcept override;
      virtual void reconnect() override;

    private:
      bool isConnected() const noexcept;
      void connect();
      void disconnect();
      int fd_;
      speed_t baudrate_;
    };
  }
}

#endif
