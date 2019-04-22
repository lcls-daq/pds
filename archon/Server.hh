#ifndef Pds_Archon_Server_hh
#define Pds_Archon_Server_hh

#include "pds/utility/EbServer.hh"
#include "pds/utility/EbCountSrv.hh"
#include "pds/utility/EbEventKey.hh"

namespace Pds {
  namespace Archon {
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
      void setFrame(uint32_t);

      void post(uint32_t, const void*, const void*, bool sync=true);

      void wait_buffers();

      void set_frame_sz(unsigned);

    private:
      Xtc _xtc;
      unsigned  _count;
      unsigned  _framesz;
      unsigned  _pending;
      uint32_t  _last_frame;
      bool      _first_frame;
      int       _pfd[4];
    };
  }
}

#endif
