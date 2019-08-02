#ifndef Pds_Jungfrau_Builder_hh
#define Pds_Jungfrau_Builder_hh

#include "pds/service/Routine.hh"

#include <zmq.h>
#include <string>
#include <vector>
#include <map>

namespace Pds {
  class Task;
  namespace Jungfrau {
    struct ZmqModuleInfo {
      std::string endpoint;
      std::string host;
      unsigned port;
    };

    class ZmqConnector {
      public:
        ZmqConnector(const char* host, const unsigned port, const unsigned range=100);
        ~ZmqConnector();
        bool listen();
        void shutdown();
        void* context() const;
        void* bind(const unsigned module_id, const char* host, unsigned port);
      private:
        bool            _bound;
        const char*     _host;
        const unsigned  _port;
        const unsigned  _range;
        void*           _context;
        void*           _socket;
        std::map<unsigned, ZmqModuleInfo> _map;
    };

    class ZmqModule {
      public:
        ZmqModule(const char* host, unsigned port,
                  const char* endpoint, void* context=NULL);
        ~ZmqModule();
        int fd() const;
        int recv();
      private:
        bool        _own_ctx;
        bool        _bound;
        const char* _host;
        unsigned    _port;
        void*       _context;
        void*       _socket;
        int         _fd;
        unsigned    _sockbuf_sz;
        unsigned    _readbuf_sz;
        unsigned    _header_sz;
        char*       _readbuf;
    };

    class ZmqDetector {
      public:
        ZmqDetector(const char* host, const unsigned port);
        ~ZmqDetector();
        void run();
        void abort();
        bool add_module(const unsigned module);
      private:
        void signal(int sig);
      private:
        const char*             _host;
        const unsigned          _port;
        void*                   _context;
        void*                   _socket;
        zmq_pollitem_t*         _pfds;
        int                     _sigfd[2];
        unsigned                _num_modules;
        std::vector<ZmqModule*> _modules;
    };

    class ZmqConnectorRoutine : public Routine {
      public:
        ZmqConnectorRoutine(ZmqConnector* con, const char* name, int priority=127);
        virtual ~ZmqConnectorRoutine();
        void enable ();
        void disable();
        void routine();
      private:
        bool          _disable;
        Task*         _task;
        ZmqConnector* _con;     
    };
  }
}

#endif
