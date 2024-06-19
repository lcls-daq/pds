#include "Server.hh"
#include "Driver.hh"
#include "FrameBuffer.hh"
#include "Errors.hh"
#include "pds/config/VimbaDataType.hh"

#include <unistd.h>
#include <sys/uio.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

using namespace Pds::Vimba;

Server::Server( const Src& client )
  : _xtc( _vimbaDataType, client ), _count(0), _last_frame(0), _first_frame(true)
{
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
  VmbFrame_t* frame = NULL;
  Xtc& xtc = *reinterpret_cast<Xtc*>(payload);
  memcpy(payload, &_xtc, sizeof(Xtc));

  int len = ::read(_pfd[0], &frame, sizeof(frame));
  if (len != sizeof(frame)) {
    fprintf(stderr, "Error: read() returned %d in %s\n", len, __FUNCTION__);
    return -1;
  }

  Camera* cam = reinterpret_cast<Camera*>(frame->context[0]);
  VimbaDataType* data = new (xtc.alloc(sizeof(VimbaDataType))) VimbaDataType(frame->frameID, frame->timestamp);

  // Check that the frame buffer is the expected size
  if (frame->imageSize != cam->payloadSize()) {
    fprintf(stderr,
            "Error: frame %llu is size %u instead of the expected %llu\n",
            frame->frameID,
            frame->imageSize,
            cam->payloadSize());
    xtc.damage.increase(Pds::Damage::UserDefined);
    xtc.damage.userBits(0x1);
  } else if (frame->receiveStatus != VmbFrameStatusComplete) {
    fprintf(stderr,
            "Error: frame %llu is damaged with status '%s'\n",
            frame->frameID,
            FrameStatusCodes::desc(frame->receiveStatus));
    xtc.damage.increase(Pds::Damage::UserDefined);
    xtc.damage.userBits(0x4);
  } else if (!FrameBuffer::copyAs16Bit(frame, xtc.alloc(FrameBuffer::sizeAs16Bit(frame)))) {
    fprintf(stderr,
            "Error: failed to convert frame %llu with pixel format %s into Mono16\n",
            frame->frameID,
            PixelFormatTypes::desc(frame->pixelFormat));
    xtc.damage.increase(Pds::Damage::UserDefined);
    xtc.damage.userBits(0x2);
  }

  // Check that the frame count is sequential
  uint64_t current_frame = data->frameid();
  if (_first_frame) {
    if (current_frame > _last_frame) {
      fprintf(stderr, "Error: expected frame %lu instead of %lu - it appears that %lu frames have been dropped\n",
              _last_frame, current_frame, current_frame - _last_frame);
    }
    _count+=(current_frame+1);
    _last_frame = current_frame;
    _first_frame = false;
  } else {
    if (current_frame - _last_frame > 1) {
      fprintf(stderr, "Error: expected frame %lu instead of %lu - it appears that %lu frames have been dropped\n",
              _last_frame+1, current_frame, (current_frame - _last_frame - 1));
    }
    _count+=(current_frame-_last_frame);
  }

  _last_frame = current_frame;

  ::write(_pfd[3], &frame, sizeof(frame));

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

void Server::resetFrame()
{
  _last_frame = 0;
  _first_frame = true;
}

bool Server::post(const void* ptr)
{
  void* ret_ptr;
  ::write(_pfd[1], &ptr, sizeof(ptr));
  // wait for the reciever to finish using the posted buffer
  ::read(_pfd[2], &ret_ptr, sizeof(ret_ptr));
  if (ptr != ret_ptr) {
    fprintf(stderr, "Error: server post did not return expected pointer %p instead of %p\n", ret_ptr, ptr);
    return false;
  }

  return true;
}
