#ifndef Pds_NsCam_Broad_hh
#define Pds_NsCam_Broad_hh

#include "pds/nscam/Device.hh"
#include "pds/nscam/Types.hh"

#include <memory>
#include <string>

namespace Pds {
  namespace NsCam {
    class Board : public Device {
    public:
      static std::shared_ptr<Board> create(BoardType btype, std::shared_ptr<Comm> comm);
      Board(BoardType btype,
            std::shared_ptr<Comm> comm,
            const std::map<std::string, uint16_t>& regnames,
            const std::map<std::string, SubRegister>& subregnames);
      virtual ~Board() = default;

      virtual void info() const;
      virtual uint32_t fpgaNum() const;
      virtual uint32_t fpgaRev() const;
      virtual BoardType type() const;
      virtual std::string name() const;
      virtual double vref() const;
      virtual uint32_t adc5_mult() const;

      virtual void initBoard() = 0;

      virtual void clearStatus() = 0;
      virtual void configADCs() = 0;

    protected:
      uint32_t fpgaNum_;
      uint32_t fpgaRev_;
      BoardType btype_;
      double vref_;
      uint32_t adc5_mult_;
      //virtual void initPots() = 0;
      //virtual void latchPots() = 0;
      //virtual void initSensor() = 0;
    };
  }
}

#endif
