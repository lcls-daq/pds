#ifndef Pds_NsCam_Gige_hh
#define Pds_NsCam_Gige_hh

#include "pds/nscam/Comm.hh"

#include "ZestETM1/ZestETM1.h"

namespace Pds {
  namespace NsCam {
    class Gige : public Comm {
    public:
      static void listDevices(unsigned long wait=1000);

      Gige(const std::string& host, unsigned short port, unsigned long timeout=10000, unsigned long wait=30);
      virtual ~Gige() noexcept;

      virtual uint32_t sendCmd(uint16_t cmd, uint16_t addr, uint32_t data=0) override;
      virtual bool openDevice() noexcept override;
      virtual bool closeDevice() noexcept override;
      virtual void info() const noexcept override;

    private:
      bool isConnected() const noexcept;
      ZESTETM1_STATUS connect() noexcept;
      ZESTETM1_STATUS disconnect() noexcept;
      ZESTETM1_STATUS readSerial(void* buffer, unsigned long len, unsigned long* nbytes) noexcept;
      ZESTETM1_STATUS writeSerial(void* buffer, unsigned long len, unsigned long* nbytes) noexcept;

      ZESTETM1_CARD_INFO  info_;
      ZESTETM1_CONNECTION conn_;
    };
  }
}

#endif
