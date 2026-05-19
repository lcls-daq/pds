#ifndef Pds_NsCam_LLNLv4_hh
#define Pds_NsCam_LLNLv4_hh

#include "pds/nscam/Board.hh"

namespace Pds {
  namespace NsCam {
    class LLNLv4 : public Board {
    public:
      LLNLv4(SensorType stype, std::shared_ptr<Comm> comm);
      virtual ~LLNLv4() = default;

      virtual void initBoard() override;
      virtual void initPots() override;

      virtual void clearStatus() override;
      virtual void configADCs() override;
    };
  }
}

#endif
