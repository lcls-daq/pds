#ifndef Pds_NsCam_LLNLv1_hh
#define Pds_NsCam_LLNLv1_hh

#include "pds/nscam/Board.hh"

namespace Pds {
  namespace NsCam {
    class LLNLv1 : public Board {
    public:
      LLNLv1(std::shared_ptr<Comm> comm);
      virtual ~LLNLv1() = default;

      virtual void initBoard() override;

      virtual void clearStatus() override;
      virtual void configADCs() override;
    };
  }
}

#endif
