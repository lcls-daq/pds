/*
 * RegisterSlaveExportFrame.cc
 *
 *  Created on: Mar 29, 2010
 *      Author: jackp
 */

#define __STDC_FORMAT_MACROS
#include "pds/pgp/RegisterSlaveExportFrame.hh"
#include "pds/pgp/PgpRSBits.hh"
#include "pds/pgp/RegisterSlaveImportFrame.hh"
#include "pds/pgp/Destination.hh"
#include "pds/pgp/Pgp.hh"
#include "pgpcard/PgpCardMod.h"
#include <PgpDriver.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>
#include <inttypes.h>
#include <linux/types.h>

namespace Pds {
  namespace Pgp {

    bool      RegisterSlaveExportFrame::_use_aes = false;
    int       RegisterSlaveExportFrame::_fd  = 0;
    bool      RegisterSlaveExportFrame::_is_datadev = false;
    unsigned  RegisterSlaveExportFrame::count = 0;
    unsigned  RegisterSlaveExportFrame::errors = 0;

    void RegisterSlaveExportFrame::FileDescr(int i, bool use_aes, bool is_datadev) {
      _is_datadev = is_datadev;
      _use_aes = use_aes; // maybe there is a way to reliably determine what driver a fd comes from but for now pass that when registering fd.
      _fd = i;
//      printf("RegisterSlaveExportFrame::FileDescr(%d)\n", _fd);
    }

    RegisterSlaveExportFrame::RegisterSlaveExportFrame(
        PgpRSBits::opcode o,
        Destination* dest,
        unsigned a,
        unsigned transID,
        uint32_t da,
        PgpRSBits::waitState w)
    {
      bits._tid     = transID & ((1<<23)-1);
      bits._waiting = w;
//      printf("RegisterSlaveExportFrame::RegisterSlaveExportFrame() lane %u offset %u\n", dest->lane(), Pgp::portOffset());
      bits._lane   = (dest->lane() & 7);
      bits.mbz     = 0;
      bits._vc     = dest->vc() & 3;
      bits.oc      = o;
      bits._addr   = a & addrMask;
      _data        = da;  // NB, for read request size of block requested minus one is placed in data field
      NotSupposedToCare = 0;
    }

    // parameter is the size of the post in number of 32 bit words
    unsigned RegisterSlaveExportFrame::post(__u32 size, bool pf) {
      struct timeval  timeout;
      int             ret;
      fd_set          fds;

      // Wait for write ready
      timeout.tv_sec=0;
      timeout.tv_usec=100000;
      FD_ZERO(&fds);
      FD_SET(_fd,&fds);

      if (_use_aes) {
        struct DmaWriteData  pgpCardTx;
        
        pgpCardTx.is32   = (sizeof(&pgpCardTx) == 4);
        pgpCardTx.flags  = 0;
        pgpCardTx.dest   = Destination::build(this->bits._lane + Pgp::portOffset(), this->bits._vc, _is_datadev);
        pgpCardTx.index  = 0;
        pgpCardTx.size   = size * sizeof(uint32_t);
        pgpCardTx.data   = (__u64)this;

        if (pf) {
          printf("RSEF::post: dest 0x%x, size 0x%x, data 0x%"PRIx64"\n",
            pgpCardTx.dest,
            pgpCardTx.size,
            pgpCardTx.data);
        }

        if ((ret = select( _fd+1, NULL, &fds, NULL, &timeout)) > 0) {
          ::write(_fd, &pgpCardTx, sizeof(pgpCardTx));
        }
      } else {
        PgpCardTx       pgpCardTx;

        pgpCardTx.model   = (sizeof(&pgpCardTx));
        pgpCardTx.cmd     = IOCTL_Normal_Write;
        pgpCardTx.pgpVc   = bits._vc;
        pgpCardTx.pgpLane = bits._lane + Pgp::portOffset();
        pgpCardTx.size    = size;
        pgpCardTx.data    = (__u32 *)this;

        if (pf) {
    	    printf("RSEF::post: model 0x%x, cmd 0x%x, vd 0x%x, lane 0x%x, size 0x%x, data 0x%p\n",
    			    pgpCardTx.model,
    			    pgpCardTx.cmd,
    			    pgpCardTx.pgpVc,
    			    pgpCardTx.pgpLane,
    			    pgpCardTx.size,
    			    pgpCardTx.data);
        }

        if ((ret = select( _fd+1, NULL, &fds, NULL, &timeout)) > 0) {
          ::write(_fd, &pgpCardTx, sizeof(pgpCardTx));
        }
      }

      if (ret <= 0) {
        if (ret < 0) {
          perror("RegisterSlaveExportFrame post select error: ");
        } else {
          printf("RegisterSlaveExportFrame post select timed out on %u\n", count);
        }
        if (errors++ < 11) print(count, size);
        return Failure;
      }
      count += 1;
      return Success;
    }

    void RegisterSlaveExportFrame::print(unsigned n, unsigned s) {
      char ocn[][20] = {"read", "write", "set", "clear"};
//      char dn[][20]  = {"Q0", "Q1", "Q2", "Q3", "Cncntr"};
      printf("Register Slave Export Frame: %u %u fd(%u)\n\t", s, n, _fd);
      printf("lane(%u), vc(%u), opcode(%s), addr(0x%x), waiting(%s), tid(0x%x), ",
          bits._lane, bits._vc, &ocn[bits.oc][0],
          bits._addr, bits._waiting ? "waiting" : "not waiting", bits._tid);
      printf("data(0x%x)\n", (unsigned)_data);
//      uint32_t* u = (uint32_t*)this;
//      printf("\t"); for (unsigned i=0;i<s;i++) printf("0x%x ", u[i]); printf("\n");
    }
  }
}
