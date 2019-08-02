#include "Builder.hh"
#include "DataFormat.hh"

#include "pds/service/Task.hh"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define ENDPOINT_LEN 64

using namespace Pds::Jungfrau;

ZmqConnector::ZmqConnector(const char* host, const unsigned port, const unsigned range) :
  _bound(false),
  _host(host),
  _port(port),
  _range(range),
  _context(zmq_ctx_new()),
  _socket(NULL)
{
  char endpoint[ENDPOINT_LEN];
  snprintf(endpoint, sizeof(endpoint), "tcp://%s:%u", host, port);
  _socket = zmq_socket(_context, ZMQ_REP);
  if (zmq_bind(_socket, endpoint) < 0) {
    fprintf(stderr, "Error: failed to bind zmq socket to endpoint %s: %s\n",
            endpoint, strerror(errno));
  } else {
    _bound = true;
  }
}

ZmqConnector::~ZmqConnector()
{
  shutdown();
}

bool ZmqConnector::listen()
{
  int nb;
  unsigned module_id;
  if (_bound) {
    nb = zmq_recv(_socket, &module_id, sizeof(module_id), 0);
    if (nb < 0) {
      fprintf(stderr, "Error: failure receiving module info request: %s\n",
              strerror(errno));
      return false;
    }

    std::map<unsigned, ZmqModuleInfo>::const_iterator it = _map.find(module_id);
    if (it != _map.end()) {
      const char* endpoint = it->second.endpoint.c_str();
      nb = zmq_send(_socket, endpoint, strlen(endpoint)+1, ZMQ_SNDMORE);
      if (nb < 0) {
        fprintf(stderr, "Error: failure sending endpoint info for module %u: %s\n",
                module_id, strerror(errno));
        return false;
      }

      const char* host = it->second.host.c_str();
      nb = zmq_send(_socket, host, strlen(host)+1, ZMQ_SNDMORE);
      if (nb < 0) {
        fprintf(stderr, "Error: failure sending host info for module %u: %s\n",
                module_id, strerror(errno));
        return false;
      }

      unsigned port = it->second.port;
      nb = zmq_send(_socket, &port, sizeof(port), 0);
      if (nb < 0) {
        fprintf(stderr, "Error: failure sending port info for module %u: %s\n",
                module_id, strerror(errno));
        return false;
      }
    } else {
      char msg[ENDPOINT_LEN];
      snprintf(msg, sizeof(msg), "module id %u does not exist!", module_id);
      nb = zmq_send(_socket, msg, strlen(msg)+1, 0);
      if (nb < 0) {
        fprintf(stderr, "Error: failure sending msg for module %u: %s\n",
                module_id, strerror(errno));
        return false;
      }
    }

    return true;
  } else {
    return false;
  }
}

void ZmqConnector::shutdown()
{
  if (_socket) {
    zmq_close(_socket);
    _socket = NULL;
  }
  if (_context) {
    zmq_ctx_destroy(_context);
    _context = NULL;
  }
}

void* ZmqConnector::context() const
{
  return _context;
}

void* ZmqConnector::bind(const unsigned module_id, const char* host, unsigned port)
{
  void* socket = NULL;
  char endpoint[ENDPOINT_LEN];
  if (_context) {
    socket = zmq_socket(_context, ZMQ_PULL);
    bool bound = false;
    for (unsigned r=0; r<_range; r++) {
      snprintf(endpoint, sizeof(endpoint), "tcp://%s:%u", _host, _port + r);
      if (zmq_bind(socket, endpoint) < 0) {
        if (errno != EADDRINUSE) {
          fprintf(stderr, "Error: bind socket failed: %s\n", strerror(errno));
          break;
        }
      } else {
        bound = true;
        break;
      }
    }
    if (bound) {
      ZmqModuleInfo info = {endpoint, host, port};
      _map[module_id] = info;
    } else {
      zmq_close(socket);
      socket = NULL;
    }
  } else {
    fprintf(stderr, "Error: bind called but there is no zmq context!\n");
  }

  return socket;
}

ZmqModule::ZmqModule(const char* host, unsigned port, const char* endpoint, void* context) :
  _own_ctx(false),
  _bound(false),
  _host(host),
  _port(port),
  _context(context),
  _socket(0),
  _fd(-1),
  _sockbuf_sz(sizeof(jungfrau_dgram)*JF_PACKET_NUM*JF_EVENTS_TO_BUFFER),
  _readbuf_sz(sizeof(jungfrau_dgram)),
  _header_sz(sizeof(jungfrau_header))
{
  _readbuf = new char[_readbuf_sz];
  if (!_context) {
    _own_ctx = true;
    _context = zmq_ctx_new();
  }
  _socket = zmq_socket(_context, ZMQ_PUSH);
  if (zmq_connect(_socket, endpoint) < 0) {
    fprintf(stderr, "Error: failed to connect to Jungfrau zmq at %s: %s\n",
            endpoint, strerror(errno));
  } else {
    hostent* entries = gethostbyname(host);
    if (entries) {
      _fd = ::socket(AF_INET, SOCK_DGRAM, 0);

      ::setsockopt(_fd, SOL_SOCKET, SO_RCVBUF, &_sockbuf_sz, sizeof(unsigned));

      unsigned addr = htonl(*(in_addr_t*)entries->h_addr_list[0]);

      sockaddr_in sa;
      sa.sin_family = AF_INET;
      sa.sin_addr.s_addr = htonl(addr);
      sa.sin_port        = htons(port);

      if (::bind(_fd, (sockaddr*)&sa, sizeof(sa)) < 0) {
        fprintf(stderr, "Error: failed to bind to Jungfrau data receiver at %s on port %d: %s\n",
                host, port, strerror(errno));
      } else {
        _bound = true;
      }
    }
  }
}

ZmqModule::~ZmqModule()
{
  if (_socket) {
    zmq_close(_socket);
    _socket = NULL;
  }
  if (_own_ctx && _context) {
    zmq_ctx_destroy(_context);
    _context = NULL;
  }
  if (_fd >= 0) {
    ::close(_fd);
    _fd = -1;
  }
  if (_readbuf) {
    delete[] _readbuf;
  }
}

int ZmqModule::fd() const
{
  return _fd;
}

int ZmqModule::recv()
{
  int nb = -1;
  ssize_t rc;
  struct sockaddr_in clientaddr;
  socklen_t clientaddrlen = sizeof(clientaddr);

  if (_bound) {
    rc = ::recvfrom(_fd, _readbuf, _readbuf_sz, 0, (struct sockaddr *)&clientaddr, &clientaddrlen);
    if (rc < (signed) _header_sz) {
      fprintf(stderr, "Error: malformed packet received by Jungfrau data receiver at %s on port %u\n",
              _host, _port);
    } else {
      nb = zmq_send(_socket, _readbuf, _header_sz, ZMQ_SNDMORE);
      if (nb >= 0) {
        nb = zmq_send(_socket, _readbuf + _header_sz, rc - _header_sz, 0);
      }
      if (nb < 0) {
        fprintf(stderr, "Error: zmq_send failure on Jungfrau data receiver at %s on port %u: %s",
                _host, _port, strerror(errno));
      }
    }
  } else {
    fprintf(stderr, "Error: recv called on Jungfrau data receiver which is not bound to port!\n");
  }

  return nb;
}

ZmqDetector::ZmqDetector(const char* host, const unsigned port) :
  _host(host),
  _port(port),
  _context(zmq_ctx_new()),
  _socket(0),
  _pfds(0),
  _num_modules(0)
{
  printf("Creating local detector segment for Jungfrau segment level at %s:%u\n",
         host, port);
  int err = ::pipe(_sigfd);
  if (err) {
    fprintf(stderr, "%s pipe error: %s\n", __FUNCTION__, strerror(errno));
  }
  _pfds = new zmq_pollitem_t[_num_modules+1];
  _pfds[_num_modules].socket   = 0;
  _pfds[_num_modules].fd       = _sigfd[0];
  _pfds[_num_modules].events   = ZMQ_POLLIN;
  _pfds[_num_modules].revents  = 0;

  _socket = zmq_socket(_context, ZMQ_REQ);
  char endpoint[strlen(host) + 30];
  snprintf(endpoint, sizeof(endpoint), "tcp://%s:%u", host, port);
  if (zmq_connect(_socket, endpoint) < 0) {
    zmq_close(_socket);
    _socket = NULL;
  }
}

ZmqDetector::~ZmqDetector()
{
  for (unsigned i=0; i<_num_modules; i++) {
    delete _modules[i];
  }
  _modules.clear();
  if (_socket) zmq_close(_socket);
  if (_context) zmq_ctx_destroy(_context);
  if (_pfds) delete[] _pfds;
}

void ZmqDetector::run()
{
  int npoll = 0;
  bool failed = false;
  while (!failed) {
    npoll = zmq_poll(_pfds, _num_modules+1, -1);
    if (npoll < 0) {
      if (errno == EINTR)
        printf("received exit signal - canceling module poller\n");
      else
        fprintf(stderr,"Error: module poller failed with error code: %s\n", strerror(errno));
      break;
    }

    if(_pfds[_num_modules].revents & ZMQ_POLLIN) {
      int signal;
      ::read(_sigfd[0], &signal, sizeof(signal));
      if (signal == JF_FRAME_WAIT_EXIT) {
        printf("received exit signal - canceling module poller\n");
        break;
      } else {
        fprintf(stderr,"Warning: received unknown signal: %d\n", signal);
      }
    }

    for (unsigned i=0; i<_num_modules; i++) {
      if (_pfds[i].revents & ZMQ_POLLIN) {
        if (_modules[i]->recv() < 0) {
          fprintf(stderr,"Error: recv for module %u failed with code: %s\n", i, strerror(errno));
          failed = true;
        }
      }
    }
  }
}

bool ZmqDetector::add_module(const unsigned module)
{
  if (_socket) {
    int nb;
    int64_t more = 0;
    size_t more_size = sizeof more;
    char endpoint[ENDPOINT_LEN];
    char host[ENDPOINT_LEN];
    unsigned port;

    nb = zmq_send(_socket, &module, sizeof(module), 0);
    if (nb < 0) {
      fprintf(stderr, "Error: failure sending info request for module %u: %s\n",
             module, strerror(errno));
      return false;
    }

    nb = zmq_recv(_socket, endpoint, sizeof(endpoint), 0);
    if (nb < 0) {
      fprintf(stderr, "Error: failure receiving endpoint info for module %u: %s\n",
             module, strerror(errno));
      return false;
    }
    nb = zmq_getsockopt(_socket, ZMQ_RCVMORE, &more, &more_size);
    if (nb < 0) {
      fprintf(stderr, "Error: failure receiving info for module %u\n", module);
      return false;
    } else if (!more) {
      fprintf(stderr, "Error: %s\n", endpoint);
      return false;
    }

    nb = zmq_recv(_socket, host, sizeof(host), 0);
    if (nb < 0) {
      fprintf(stderr, "Error: failure receiving host info for module %u: %s\n",
             module, strerror(errno));
      return false;
    }

    nb = zmq_recv(_socket, &port, sizeof(port), 0);
    if (nb < 0) {
      fprintf(stderr, "Error: failure receiving port info for module %u: %s\n",
             module, strerror(errno));
      return false;
    }

    printf("Adding new Jungfrau module listening at %s:%u and forwarding data to zmq endpoint %s\n",
           host, port, endpoint);

    ZmqModule* mod = new ZmqModule(host, port, endpoint, _context);
    _modules.push_back(mod);

    zmq_pollitem_t* pfds = new zmq_pollitem_t[_num_modules+2];
    for(unsigned i=0; i<_num_modules; i++)
      pfds[i] = _pfds[i];
    // add new module
    pfds[_num_modules].socket   = 0;
    pfds[_num_modules].fd       = mod->fd();
    pfds[_num_modules].events   = ZMQ_POLLIN;
    pfds[_num_modules].revents  = 0;
    // add signal fd
    pfds[_num_modules+1] = _pfds[_num_modules];
    // cleanup old poller array
    delete[] _pfds;
    _pfds = pfds;
    // increment module counter
    _num_modules++;

    return true;
  } else {
    fprintf(stderr, "Error: add_module called but Zmq detector is not connected to segment level!\n");
  }

  return false;
}

void ZmqDetector::signal(int sig)
{
  ::write(_sigfd[1], &sig, sizeof(sig));
}

void ZmqDetector::abort()
{
  signal(JF_FRAME_WAIT_EXIT);
}

ZmqConnectorRoutine::ZmqConnectorRoutine(Jungfrau::ZmqConnector* con, const char* name, int priority) :
  _disable(true),
  _task(new Task(TaskObject(name, priority))),
  _con(con)
{}

ZmqConnectorRoutine::~ZmqConnectorRoutine()
{
  if(_task) _task->destroy();
}

void ZmqConnectorRoutine::enable ()
{
  _disable=false;
  _task->call(this);
}

void ZmqConnectorRoutine::disable()
{
  _disable=true;
}

void ZmqConnectorRoutine::routine()
{
  if (!_disable) {
    if (_con->listen()) {
      _task->call(this);
    }
  }
}

#undef ENDPOINT_LEN
