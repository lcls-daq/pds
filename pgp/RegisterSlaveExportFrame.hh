/*
 * RegisterSlaveExportFrame.hh
 *
 *  Created on: Mar 29, 2010
 *      Author: jackp
 */

#ifndef PGPREGISTERSLAVEEXPORTFRAME_HH_
#define PGPREGISTERSLAVEEXPORTFRAME_HH_

#include <stdint.h>
#include <linux/types.h>
#include <fcntl.h>
#include "pds/pgp/PgpRSBits.hh"
#include "pds/pgp/Destination.hh"
#include "pgpcard/PgpCardMod.h"

namespace Pds {
  namespace Pgp {

    class RegisterSlaveExportFrame {
      public:
        enum {Success=0, Failure=1};
        enum masks {addrMask=(1<<30)-1};

        RegisterSlaveExportFrame(
            PgpRSBits::opcode,   // opcode
            Destination* dest,   // Destination
            unsigned,            // address
            unsigned,            // transaction ID
            uint32_t=0,          // data
            PgpRSBits::waitState=PgpRSBits::notWaiting);

        ~RegisterSlaveExportFrame() {};

      public:
        static unsigned    count;
        static unsigned    errors;
        static void FileDescr(int i, bool use_aes=false, bool is_datadev=false);

      private:
        static bool            _is_datadev;
        static bool            _use_aes;
        static int             _fd;

      public:
        unsigned tid()                            {return bits._tid;}
        void waiting(PgpRSBits::waitState w)      {bits._waiting = w;}
        uint32_t* array()                         {return (uint32_t*)&_data;}
        unsigned post(__u32, bool pf=false);  // the size of the entire header + payload in number of 32 bit words
        void print(unsigned = 0, unsigned = 4);


      public:
        PgpRSBits bits;
        uint32_t _data;
        uint32_t NotSupposedToCare;
    };

  }
}

#endif /* PGPREGISTERSLAVEEXPORTFRAME_HH_ */
