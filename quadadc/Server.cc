#include "pds/quadadc/Server.hh"
#include "pds/quadadc/RxDesc.hh"

#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/psddl/generic1d.ddl.h"
#include <stdint.h>
#include <stdio.h>
static const unsigned maxSize = 32000*4;

using namespace Pds::QuadAdc;

Server::Server(const DetInfo& info,
               int            fdes) :
  _xtc(Pds::TypeId(Pds::TypeId::Id_Generic1DData,0),
       info),
  _fd (fdes),
  _evBuffer(new char[maxSize*4])
{
  fd(fdes);
}

Server::~Server()
{
  delete[] _evBuffer;
}

int Server::fetch(char* payload, int flags)
{
  static unsigned sec = 0;

  Pds::Tpr::RxDesc* desc = 
    new Pds::Tpr::RxDesc(reinterpret_cast<uint32_t*>(_evBuffer),
                         maxSize);
  ssize_t nb = read(_fd, desc, sizeof(*desc));
  if (nb >= 0) {
    uint32_t* p = (uint32_t*)_evBuffer;
    if (1) {
      int len = ((p[0]&0xffffff)-8)*4;
      _fid = p[4];
      // Hack timing system has error

      //_fid = p[4]+1;

      if (_fid == (1<<17)-32 )
	_fid = 0;
      
#if 1
      if (p[2]!=sec) {
        printf("--");
        for(unsigned i=0; i<8; i++)
          printf("%08x ",p[i]);
        printf("--\n");

        const uint16_t* q = reinterpret_cast<uint16_t*>(&p[8]);
        for(unsigned i=0; i<4; i++) {
          for(unsigned j=0; j<16; j++,q++)
            printf("%04x ",*q);
          printf("\n");
        }
        sec = p[2];
      }
#endif

      const unsigned EXTRA_SAMPLES=80;
#if 0
      //
      //  ADC was configured to deliver extra samples so we could correct the waveform
      //  depending upon the location of the trigger bit (sync).  
      //  Make the correction and remove the extra samples here.  Move this to firmware after beamtest.
      //
      uint32_t sync = (p[5]&0xfff)>>2;

      uint16_t* r = reinterpret_cast<uint16_t*>(&p[8]);
      //  Leave ChA untouched (starts at "r")
      unsigned nlen = len - EXTRA_SAMPLES*8;  // remove EXTRA SAMPLES
      uint16_t* dst = r+nlen/8;  //  New location for ChB
      uint16_t* src = r+sync;    //  Start of corrected ChA
      memcpy(dst, src, nlen/4);  //  Copy corrected ChA onto ChB
      src = r+len/8;             //  Start of uncorrected ChB (skip)
      dst += nlen/8;             //  New location for ChC
      src += len/8;              //  Start of uncorrected ChC
      memcpy(dst, src, nlen/4);  //  Copy ChC into new location
      src += sync;               //  Start of corrected ChC
      dst += nlen/8;             //  New location for ChD
      memcpy(dst, src, nlen/4);  //  Copy corrected ChC onto ChD

      const uint8_t* q = reinterpret_cast<const uint8_t*>(r);
      Xtc* tc = new(payload) Xtc(_xtc);
      new (tc->alloc(sizeof(Pds::Generic1D::DataV0)+nlen)) 
        Pds::Generic1D::DataV0(nlen,q);
      return _xtc.extent = tc->extent;
#else
      //
      //  Header fields: p[0]        = size in dwords
      //                 p[1][23:16] = channel mask
      //                 p[1][13: 0] = samples/16
      //                 p[2]        = sec
      //                 p[3]        = nsec
      //                 p[4]        = pulseId
      //                 p[5]        = sync sample
      //                 p[6]        = 119M clks since last event
      //                 p[7]        = 0
      //
      const uint8_t* q = reinterpret_cast<const uint8_t*>(&p[8]);
      Xtc* tc = new(payload) Xtc(_xtc);
      new (tc->alloc(sizeof(Pds::Generic1D::DataV0)+len)) 
        Pds::Generic1D::DataV0(len,q);
      return _xtc.extent = tc->extent;
#endif
    }
  }

  return -1;
}

int  Server::pend(int)
{ return -1; }

void Server::dump(int) const
{
}

bool Server::isValued() const { return true; }

const Pds::Src& Server::client() const { return _xtc.src; }

const Pds::Xtc& Server::xtc() const { return _xtc; }

bool Server::more() const { return false; }

unsigned Server::length() const { return _xtc.extent; }

unsigned Server::offset() const { return 0; }

