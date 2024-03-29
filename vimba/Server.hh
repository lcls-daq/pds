#ifndef Pds_Vimba_Server_hh
#define Pds_Vimba_Server_hh

#include "pds/utility/EbServer.hh"
#include "pds/utility/EbCountSrv.hh"
#include "pds/utility/EbEventKey.hh"

namespace Pds {
  namespace Vimba {
    class Server : public EbServer,
                   public EbCountSrv {

    public:
      Server( const Src& client );
      virtual ~Server() {}

      //  Eb interface
      void       dump ( int detail ) const {}
      bool       isValued( void ) const    { return true; }
      const Src& client( void ) const      { return _xtc.src; }

      //  EbSegment interface
      const Xtc& xtc( void ) const    { return _xtc; }
      unsigned   offset( void ) const { return sizeof(Xtc); }
      unsigned   length() const       { return _xtc.extent; }

      //  Eb-key interface
      EbServerDeclare;

      //  Server interface
      int pend( int flag = 0 ) { return -1; }
      int fetch( char* payload, int flags );

      unsigned count() const;
      void resetCount();
      void resetFrame();

      bool post(const void*);

    private:
      Xtc       _xtc;
      unsigned  _count;
      uint64_t  _last_frame;
      bool      _first_frame; 
      int       _pfd[4];
    };
  }
}

#endif
