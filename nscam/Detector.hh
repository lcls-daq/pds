#ifndef Pds_NsCam_Detector_hh
#define Pds_NsCam_Detector_hh

#include "pds/nscam/Comm.hh"
#include "pds/nscam/Board.hh"

namespace Pds {
  namespace NsCam {
    class Detector {
    public:
      static void listDevices();
      Detector(const std::string& host,
               unsigned short port,
               CommType ctype,
               BoardType btype);
      virtual ~Detector();

      void interfaceInfo() const;
      void boardInfo() const;
      std::string boardName() const;
      uint32_t boardFpgaNum() const;
      uint32_t boardFpgaRev() const;
    private:
      std::shared_ptr<Comm> comm_;
      std::shared_ptr<Board> board_;
    };
  }
}

#endif
