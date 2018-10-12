/*-------------------------------------------------------------------------------
-- Title      : 
-------------------------------------------------------------------------------
-- File       : ServerSim.hh
-- Author     : Matt Weaver <weaver@slac.stanford.edu>
-- Company    : SLAC National Accelerator Laboratory
-- Created    : 2018-09-28
-- Last update: 2018-09-17
-- Platform   : 
-- Standard   : C++
-------------------------------------------------------------------------------
-- Description: 
-------------------------------------------------------------------------------
-- This file is part of 'LCLS DAQ'.
-- It is subject to the license terms in the LICENSE.txt file found in the 
-- top-level directory of this distribution and at: 
--    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
-- No part of 'LCLS DAQ', including this file, 
-- may be copied, modified, propagated, or distributed except according to 
-- the terms contained in the LICENSE.txt file.
-------------------------------------------------------------------------------*/

#ifndef Pds_Epix10ka2m_ServerSim_hh
#define Pds_Epix10ka2m_ServerSim_hh

#include "pds/epix10ka2m/Server.hh"
#include "pds/utility/BldSequenceSrv.hh"

namespace Pds {

  namespace Epix10ka2m {
    class Configurator;

    class ServerSim : public Pds::Epix10ka2m::Server,
                      public BldSequenceSrv {
    public:
      ServerSim( Pds::Epix10ka2m::Server* hdw );
      ServerSim( ServerSim*      bwd );
      virtual ~ServerSim() {}
      
      //  Eb-key interface
      EbServerDeclare;
      unsigned fiducials() const;

      //  Eb interface
      void       dump    ( int  ) const;
      bool       isValued( void ) const;
      const Src& client  ( void ) const;
      
      //  EbSegment interface
      const Xtc& xtc     ( void ) const;
      unsigned   length  (      ) const;

      //  Server interface
      int        pend ( int flag = 0 );
      int        fetch( char* payload, int flags );
      bool       more() const;

      void     allocated  (const Allocate& a);
      unsigned configure(const Pds::Epix::PgpEvrConfig&,
                         const Epix10kaQuadConfigType&, 
                         Pds::Epix::Config10ka*,
                         bool forceConfig = false);
      unsigned unconfigure(void);
      
      void     enable();
      void     disable();
      bool     g3sync() const;
      void     ignoreFetch(bool);
      Configurator* configurator();
      void          dumpFrontEnd();
      void          die         ();
      //
      void     post(char*, int);

    private:
      Pds::Epix10ka2m::Server*       _hdw;
      ServerSim*                     _bwd;
      ServerSim*                     _fwd;
      int                            _pfd[2];
    };
  };
};

#endif
