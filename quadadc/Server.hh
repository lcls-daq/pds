#ifndef Pds_QuadAdc_Server_hh
#define Pds_QuadAdc_Server_hh

#include "pds/utility/EbServer.hh"
#include "pds/utility/AnySequenceSrv.hh"
#include "pds/utility/BldSequenceSrv.hh"

#include "pds/utility/EbEventKey.hh"

#include <vector>

namespace Pds {
  class DetInfo;
  namespace QuadAdc {
    class Server : public EbServer {
    public:
      Server(const DetInfo&, int fd);
      ~Server();
    public:
      // Server interface
      int         fetch   (char*,int);
      int         pend    (int);
      void        dump    (int) const;
      bool        isValued() const;
      const Src&  client  () const;
      const Xtc&  xtc     () const;
      bool        more    () const;
      unsigned    length  () const;
      unsigned    offset  () const;
    protected:
      Xtc      _xtc;
      unsigned _fid;
      int      _fd;
      char*    _evBuffer;
    };

    class SeqMatch : public Server,
                     public BldSequenceSrv {
    public:
      SeqMatch(const DetInfo& info, int fd) : Server(info,fd) {}
      ~SeqMatch() {}
    public:
      EbServerDeclare;
    public:
      unsigned fiducials() const { return _fid; }
    };

    class AnyMatch : public Server,
                     public AnySequenceSrv {
    public:
      AnyMatch(const DetInfo& info, int fd) : Server(info,fd) {}
      ~AnyMatch() {}
    public:
      EbServerDeclare;
    };
  };
};

#endif
