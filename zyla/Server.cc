#include "Server.hh"
#include "pds/config/ZylaDataType.hh"

#include <unistd.h>
#include <sys/uio.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

using namespace Pds::Zyla;

Server::Server( const Src& client )
  : _xtc( _zylaDataType, client ), _count(0), _framesz(0)
{
  _xtc.extent = sizeof(ZylaDataType) + sizeof(Xtc);
  int err = ::pipe(_pfd);
  if (err) {
    fprintf(stderr, "%s pipe error: %s\n", __FUNCTION__, strerror(errno));
  } else {
    fd(_pfd[0]);
  }
  // Create a second pipe for acknowledging post
  err = ::pipe(&_pfd[2]);
  if (err) {
    fprintf(stderr, "%s pipe error: %s\n", __FUNCTION__, strerror(errno));
  }
}

int Server::fetch( char* payload, int flags )
{
  void* ptr;
  Xtc& xtc = *reinterpret_cast<Xtc*>(payload);
  memcpy(payload, &_xtc, sizeof(Xtc));

  int len = ::read(_pfd[0], &ptr, sizeof(ptr));
  if (len != sizeof(ptr)) {
    fprintf(stderr, "Error: read() returned %d in %s\n", len, __FUNCTION__);
    return -1;
  }

  memcpy(xtc.payload(), ptr, sizeof(ZylaDataType) + _framesz);

  _count++;

  ::write(_pfd[3], &ptr, sizeof(ptr));

  return xtc.extent;
}

unsigned Server::count() const
{
  return _count - 1;
}

void Server::resetCount()
{
  _count = 0;
}

void Server::post(const void* ptr)
{
  void* ret_ptr;
  ::write(_pfd[1], &ptr, sizeof(ptr));
  // wait for the reciever to finish using the posted buffer
  ::read(_pfd[2], &ret_ptr, sizeof(ret_ptr));
  if (ptr != ret_ptr) {
    fprintf(stderr, "Error: server post did not return expected pointer %p instead of %p\n", ret_ptr, ptr);
  }
}

void Server::set_frame_sz(size_t sz)
{
  _framesz = sz;
  _xtc.extent = sizeof(ZylaDataType) + sizeof(Xtc) + sz;
}
