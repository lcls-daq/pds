#include "FrameBuffer.hh"
#include "Driver.hh"
#include "Server.hh"
#include "Errors.hh"

#include "vimba/include/VmbTransform.h"

#include <algorithm>
#include <cstdio>

using namespace Pds::Vimba;

FrameBuffer::FrameBuffer(unsigned nbuffers, Camera* cam, Server* srv):
  _nbuffers(nbuffers),
  _cam(cam),
  _srv(srv),
  _frames(new VmbFrame_t[nbuffers]),
  _buffer(NULL),
  _buffer_sz(0)
{}

FrameBuffer::~FrameBuffer()
{
  if (_frames) {
    delete[] _frames;
  }
  if (_buffer) {
    delete[] _buffer;
  }
}

void FrameBuffer::configure()
{
  if (_cam) {
    VmbInt64_t payloadSize = _cam->payloadSize();

    // allocate memory for buffers if needed
    if (_buffer_sz < (_nbuffers * payloadSize)) {
      if (_buffer) delete[] _buffer;
      _buffer = new char[_nbuffers * payloadSize];
      _buffer_sz = _nbuffers * payloadSize;
    }

    // initialize the frames with buffer info
    for (unsigned n=0; n<_nbuffers; n++) {
      _frames[n].buffer = _buffer + (payloadSize * n);
      _frames[n].bufferSize = payloadSize;
      _frames[n].context[0] = _cam;
      _frames[n].context[1] = _srv;
    }

    // register allocated frames
    _cam->registerFrames(_frames, _nbuffers);
    // start capture engine
    _cam->captureStart();
    // queue frames
    _cam->queueFrames(_frames, _nbuffers, frameCallBack);

    // show readout buffer information
    printf("Buffer information:\n");
    printf("  Driver Buffer Count:    %lld\n", _cam->maxDriverBuffersCount());
    printf("  Stream Buffer Mode:     %s\n",   _cam->streamBufferHandlingMode());
    printf("  Stream Buffer Minimum:  %lld\n", _cam->streamAnnounceBufferMinimum());
    printf("  Stream Buffer Count:    %lld\n", _cam->streamAnnouncedBufferCount());
  }
}

void FrameBuffer::unconfigure()
{
  if (_cam) {
    // end capture
    _cam->captureEnd();
    // flush the frame queue
    _cam->flushFrames();
    // unregister the frames
    _cam->unregisterAllFrames();
  }
}

bool FrameBuffer::enable()
{
  if (_cam) {
    return _cam->acquisitionStart();
  } else {
    return false;
  }
}

bool FrameBuffer::disable()
{
  if (_cam) {
    return _cam->acquisitionStop();
  } else {
    return false;
  }
}

VmbUint32_t FrameBuffer::sizeAs16Bit(VmbFrame_t* frame)
{
  return frame->width * frame->height * sizeof(VmbMono16_t);
}

bool FrameBuffer::copyAs16Bit(VmbFrame_t* frame, void* buffer)
{
  switch (frame->pixelFormat) {
    case VmbPixelFormatMono8:
    case VmbPixelFormatMono10:
    case VmbPixelFormatMono12:
    case VmbPixelFormatMono14:
    case VmbPixelFormatMono16:
      return copyAs16BitStd(frame, buffer);
    case VmbPixelFormatMono10p:
    case VmbPixelFormatMono12Packed:
    case VmbPixelFormatMono12p:
      return copyAs16BitVmb(frame, buffer);
    default:
      fprintf(stderr, "%s error: frame has an unsupported pixel encoding - %s\n", __FUNCTION__, PixelFormatTypes::desc(frame->pixelFormat));
      return false;
  }
  return true;
}

bool FrameBuffer::copyAs16BitVmb(VmbFrame_t* frame, void* buffer)
{
  VmbError_t err = VmbErrorSuccess;
  VmbImage src;
  VmbImage dest;

  // set size member for verification inside API
  src.Size = sizeof(src);
  src.Data = frame->buffer;
  dest.Size = sizeof(dest);
  dest.Data = buffer;

  err = VmbSetImageInfoFromPixelFormat(frame->pixelFormat, frame->width, frame->height, &src);
  if (err != VmbErrorSuccess) {
    fprintf(stderr, "%s error: failed to set input image info - %s\n", __FUNCTION__, ErrorCodes::desc(err));
    return false;
  }

  err = VmbSetImageInfoFromInputImage(&src, VmbPixelLayoutMono, 16, &dest);
  if (err != VmbErrorSuccess) {
    fprintf(stderr, "%s error: failed to set output image info - %s\n", __FUNCTION__, ErrorCodes::desc(err));
    return false;
  }

  err = VmbImageTransform (&src, &dest, NULL, 0);
  if (err != VmbErrorSuccess) {
    fprintf(stderr, "%s error: failed to transform image - %s\n", __FUNCTION__, ErrorCodes::desc(err));
    return false;
  } else {
    return true;
  }
}

bool FrameBuffer::copyAs16BitStd(VmbFrame_t* frame, void* buffer)
{
  char* frame_start = (char*) frame->buffer;
  char* frame_end = frame_start + frame->imageSize;
  if (frame->pixelFormat & VmbPixelOccupy16Bit) {
    std::copy((uint16_t*) frame_start, (uint16_t*) frame_end, (uint16_t*) buffer);
    return true;
  } else if (frame->pixelFormat & VmbPixelOccupy8Bit) {
    std::copy((uint8_t*) frame_start, (uint8_t*) frame_end, (uint16_t*) buffer);
    return true;
  } else {
    fprintf(stderr, "%s error: frame has an unsupported pixel encoding - %s\n", __FUNCTION__, PixelFormatTypes::desc(frame->pixelFormat));
    return false;
  }
}

void VMB_CALL FrameBuffer::frameCallBack(const VmbHandle_t hcam, VmbFrame_t* ptr)
{
  Server* srv = reinterpret_cast<Server*>(ptr->context[1]);
  if (srv->post(ptr)) {
    VmbCaptureFrameQueue(hcam, ptr, frameCallBack);
  }
}
