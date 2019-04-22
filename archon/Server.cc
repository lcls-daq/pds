#include "Server.hh"
#include "pds/config/ArchonDataType.hh"

#include <unistd.h>
#include <sys/uio.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

struct server_post {
  uint32_t frame;
  const void* header;
  const void* data;
};

using namespace Pds::Archon;

Server::Server( const Src& client )
  : _xtc( _archonDataType, client ), _count(0), _framesz(0), _pending(0), _last_frame(0), _first_frame(true)
{
  _xtc.extent = sizeof(ArchonDataType) + sizeof(Xtc);
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
  struct server_post info;
  Xtc& xtc = *reinterpret_cast<Xtc*>(payload);
  memcpy(payload, &_xtc, sizeof(Xtc));

  int len = ::read(_pfd[0], &info, sizeof(info));
  if (len != sizeof(info)) {
    fprintf(stderr, "Error: read() returned %d in %s\n", len, __FUNCTION__);
    return -1;
  }

  memcpy(xtc.payload(), info.header, sizeof(ArchonDataType));
  memcpy(xtc.payload() + sizeof(ArchonDataType), info.data, _framesz);

  // Check that the frame count is sequential
  if (_first_frame) {
    _last_frame = info.frame;
    _first_frame = false;
    _count++;
  } else {
    if (info.frame - _last_frame > 1) {
      fprintf(stderr, "Error: expected frame %u instead of %u - it appears that %u frames have been dropped\n",
              _last_frame+1, info.frame, (info.frame - _last_frame - 1));
    }
    _count+=(info.frame-_last_frame);
  }

  _last_frame = info.frame;

  ::write(_pfd[3], &info.data, sizeof(info.data));

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

void Server::setFrame(uint32_t frame)
{
  _last_frame = frame;
  _first_frame = false;
}

void Server::post(uint32_t frame, const void* hdr, const void* ptr, bool sync)
{
  void* ret_ptr;
  struct server_post info = { frame, hdr, ptr };
  ::write(_pfd[1], &info, sizeof(info));
  if (sync) {
    // wait for the reciever to finish using the posted buffer
    ::read(_pfd[2], &ret_ptr, sizeof(ret_ptr));
    if (ptr != ret_ptr) {
      fprintf(stderr, "Error: server post did not return expected pointer %p instead of %p\n", ret_ptr, ptr);
    }
  } else {
    _pending++;
  }
}

void Server::wait_buffers()
{
  void* ret_ptr;
  while (_pending) {
    ::read(_pfd[2], &ret_ptr, sizeof(ret_ptr));
    _pending--;
  }
}

void Server::set_frame_sz(unsigned sz)
{
  _framesz = sz;
  _xtc.extent = sizeof(ArchonDataType) + sizeof(Xtc) + sz;
}
