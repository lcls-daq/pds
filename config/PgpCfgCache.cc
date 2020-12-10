#include "PgpCfgCache.hh"

#include "pds/xtc/InDatagram.hh"
#include "pds/xtc/Datagram.hh"
#include "pds/config/EvrCfgCache.hh"

using namespace Pds;

PgpCfgCache::PgpCfgCache(const Src&    src,
                      const TypeId& id,
                      int           size) :
  CfgCache(src, id, size),
  _evr(new EvrCfgCache())
{
}

PgpCfgCache::PgpCfgCache(const Src&    dbsrc,
                         const Src&    xtcsrc,
                         const TypeId& id,
                         int           size) :
  CfgCache(dbsrc, xtcsrc, id, size),
  _evr(new EvrCfgCache())
{
}

PgpCfgCache::~PgpCfgCache()
{
  if (_evr)
    delete _evr;
}

void PgpCfgCache::init(const Allocation& alloc)
{
  _evr->init(alloc);
  CfgCache::init(alloc);
}

int PgpCfgCache::fetch(const Transition* tr)
{
  _evr->fetch(tr);
  return CfgCache::fetch(tr);
}

unsigned PgpCfgCache::code() const
{
  return _evr->code();
}

unsigned PgpCfgCache::group() const
{
  return _evr->group();
}

