#include "pds/nscam/Server.hh"
#include "pds/config/UxiDataType.hh"

#include <unistd.h>
#include <sys/uio.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

using namespace Pds::NsCam;

Server::Server( const Src& client )
  : _xtc( _uxiDataType, client ), _count(0), _framesz(0), _last_frame(0), _first_frame(true)
{
  _xtc.extent = sizeof(UxiDataType) + sizeof(Xtc);
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
  FrameMetaData ptr;
  Xtc& xtc = *reinterpret_cast<Xtc*>(payload);
  memcpy(payload, &_xtc, sizeof(Xtc));

  int len = ::read(_pfd[0], &ptr, sizeof(ptr));
  if (len != sizeof(ptr)) {
    fprintf(stderr, "Error: read() returned %d in %s\n", len, __FUNCTION__);
    return -1;
  }

  UxiDataType& uxidata = *new(xtc.payload()) UxiDataType(ptr.acq_count, ptr.timestamp, ptr.temperature);
  memcpy(xtc.payload() + sizeof(UxiDataType), ptr.data, _framesz);

  uint32_t current_frame = uxidata.acquisitionCount();
  // Check that the frame count is sequential
  if (_first_frame) {
    _last_frame = current_frame;
    _first_frame = false;
    _count++;
  } else {
    if (current_frame - _last_frame > 1) {
      fprintf(stderr, "Error: expected frame %u instead of %u - it appears that %u frames have been dropped\n", _last_frame+1, current_frame, (current_frame - _last_frame - 1));
    }
    _count+=(current_frame-_last_frame);
  }

  _last_frame = current_frame;

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

void Server::setFrame(uint32_t frame)
{
  _last_frame = frame;
  _first_frame = false;
}

void Server::post(const FrameMetaData& ptr)
{
  FrameMetaData ret_ptr;
  ::write(_pfd[1], &ptr, sizeof(ptr));
  // wait for the reciever to finish using the posted buffer
  ::read(_pfd[2], &ret_ptr, sizeof(ret_ptr));
  if (ptr.data != ret_ptr.data) {
    fprintf(stderr, "Error: server post did not return expected pointer %p instead of %p\n", ret_ptr.data, ptr.data);
  }
}

void Server::set_frame_sz(unsigned sz)
{
  _framesz = sz;
  _xtc.extent = sizeof(UxiDataType) + sizeof(Xtc) + sz;
}
