#include "ZcpDatagramIterator.hh"

#include <stdio.h>
#include <errno.h>
#include <string.h>

//#define VERBOSE

using namespace Pds;

ZcpDatagramIterator::ZcpDatagramIterator( const ZcpDatagram& dg) :
  _size  (dg.datagram().xtc.sizeofPayload()),
  _offset(0)
{
#ifdef VERBOSE
  printf("Creating ZcpDgIter %p  from dg size %d\n",
	 this,_size);
#endif
  ZcpFragment  zcp;
  ZcpDatagram& zdg(const_cast<ZcpDatagram&>(dg));
  int remaining(_size);
  while( remaining ) {
    int len = zdg._stream.remove(_fragment, remaining);
#ifdef VERBOSE
    printf("Removed %d/%d bytes from input\n",len,remaining);
#endif
    int copied = zcp.copy(_fragment, len);
#ifdef VERBOSE
    printf("Copied %d/%d bytes\n",copied,len);
#endif
    int err;
    if ( (err = zdg._stream.insert(_fragment, len)) != len ) {
      if (err < 0)
	printf("ZDI error scanning input stream : %s\n", strerror(errno));
      else if (err != len)
	printf("ZDI error scanning input stream %d/%d bytes\n", err,len);
    }
    if ( (err = _stream.insert(zcp, len)) != len ) {
      if (err < 0)
	printf("ZDI error copying input stream : %s\n", strerror(errno));
      else if (err != len)
	printf("ZDI error copying input stream %d/%d bytes\n", err,len);
    }
    remaining -= len;
  }
}

ZcpDatagramIterator::~ZcpDatagramIterator()
{
  _iovbuff.unmap(_stream);
}

//
// advance "len" bytes (without mapping)
//
int ZcpDatagramIterator::skip(int len)
{
  if (_offset + len > _size) {
    printf("ZDI::skip(%d+%d) exceeds datagram extent(%d) ...reducing\n",
	   len, _offset, _size);
    len = _size - _offset;
  }

  int remaining(len);
#ifdef VERBOSE
  printf("skipping %d bytes .. mapped\n",remaining);
#endif
  // skip over mapped bytes first
  remaining -= _iovbuff.remove(len);  

  // skip over unmapped bytes
#ifdef VERBOSE
  printf("skipping %d bytes .. unmapped\n",remaining);
#endif

  /***
   ***  Once kmemory_skip is implemented
   **/
  _stream.remove(remaining);
  remaining -= _iovbuff.remove(remaining);

#ifdef VERBOSE
  printf("skipping left %d bytes\n",remaining);
#endif
  _offset += (len-remaining);

  return len-remaining;
}

//
// read (and map) "len" bytes into iov array and advance
//
int ZcpDatagramIterator::read(iovec* iov, int maxiov, int len)
{
  if (_offset + len > _size) {
    printf("ZDI::read(%d+%d) exceeds datagram extent(%d) ...reducing\n",
	   len, _offset, _size);
    len = _size - _offset;
  }

#ifdef VERBOSE
  printf("read %d bytes\n",len);
#endif
  //  Pull enough data into the IovBuffer
  while (len > _iovbuff.bytes()) {
    _iovbuff.insert(_stream, len-_iovbuff.bytes());
  }

  _offset += len;

  //  Map the data into user-space and return the user iov
  return _iovbuff.remove(iov,maxiov,len);
}


ZcpDatagramIterator::IovBuffer::IovBuffer() :
  _bytes  (0),
  _iiov   (0),
  _iiovlen(0),
  _niov   (0)
{
  _iovs[_iiov].iov_len = 0;
}

ZcpDatagramIterator::IovBuffer::~IovBuffer()
{
}

void ZcpDatagramIterator::IovBuffer::unmap(KStream& stream)
{
  stream.unmap(_iovs, _niov);
}

//
//  Here we are skipping over buffers that were already mapped
//
int ZcpDatagramIterator::IovBuffer::remove(int bytes)
{
#ifdef VERBOSE
  printf("ZDI remove %d/%d bytes\n",bytes,_bytes);
#endif
  int len(bytes);
  if (_iiovlen) {
    len -= _iiovlen;
    _iiovlen = 0;
    _iiov++;
  }
  while( len>0 && _iiov<_niov ) {
    len -= _iovs[_iiov].iov_len;
    _iiov++;
  }
  if (len < 0) {
    _iiovlen = -len;
    _iiov--;
    len = 0;
  }
  bytes  -= len;
  _bytes -= bytes;
  return bytes;
}

//
//  Pass these mapped buffers to the user's iovs
//
int ZcpDatagramIterator::IovBuffer::remove(iovec* iov, int maxiov, int bytes)
{
#ifdef VERBOSE
  printf("ZDI remove %d/%d bytes\n",bytes,_bytes);
#endif
  iovec* iovend = iov+maxiov;
  int len = bytes;
  if (_iiovlen) {
    iov->iov_base = (char*)_iovs[_iiov].iov_base + 
      _iovs[_iiov].iov_len - _iiovlen;
    iov->iov_len = _iiovlen;
    len -= _iiovlen;
    _iiovlen = 0;
    _iiov++;
    iov++;
  }
  while( len>0 && iov<iovend ) {
    iov->iov_base = _iovs[_iiov].iov_base;
    iov->iov_len  = _iovs[_iiov].iov_len;
    len          -= _iovs[_iiov].iov_len;
    iov++;
    _iiov++;
  }
  if (len < 0) {
    _iiovlen = -len;
    _iiov--;
    iov--;
    iov->iov_len += len;
  }
  _bytes -= bytes;
  return bytes;
}

//
//  Map more data into the iovs
//
int ZcpDatagramIterator::IovBuffer::insert(KStream& stream, int bytes)
{
#ifdef VERBOSE
  printf("ZDI::IovBuffer::insert %d bytes\n",bytes);
#endif
  int obytes(_bytes);

  do {
    //  Try the minimum number of iovs (4kB page size)
    int niov = (bytes >> 12)+1;
    if (_niov+niov > MAXIOVS)
      niov = MAXIOVS - _niov;

    niov = stream.map(&_iovs[_niov],niov);
    if (niov < 0) {
      printf("ZDI::IovBuffer::insert kmem_read failed %s(%d) : iov %p/%d maxiov %d\n",
	     strerror(errno),errno,&_iovs[_niov],_niov,MAXIOVS-_niov);
      break;
    }
#ifdef VERBOSE
    printf("ZDI::insert mapped %d iovs\n",niov);
    for(int k=0; k<niov; k++)
      printf("ZDI::iov %p/%d\n",_iovs[_niov+k].iov_base,_iovs[_niov+k].iov_len);
#endif

    int len  = 0;
    while( niov-- )
      len += _iovs[_niov++].iov_len;
    if (_niov == MAXIOVS)
      printf("ZcpDatagramIterator::IovBuffer exhausted %d iovs\n",_niov);
    bytes  -= len;
    _bytes += len;
  } while (bytes > 0);

  return _bytes-obytes;
}
