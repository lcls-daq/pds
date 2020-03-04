#include "pds/config/AliasFactory.hh"

#include "pdsdata/xtc/SegmentInfo.hh"

using namespace Pds;

AliasFactory::AliasFactory() {}

AliasFactory::~AliasFactory() {}

void        AliasFactory::insert(const Alias::SrcAlias& alias)
{
  _insert_parent(alias);
  _list.push_back(alias);
  _list.sort();
  _list.unique();
}

void        AliasFactory::_insert_parent(const Alias::SrcAlias& alias)
{
  bool found = false;
  const SegmentInfo& info = reinterpret_cast<const SegmentInfo&>(alias.src());
  if (info.isChild()) {
    std::string alias_name(alias.aliasName());
    SegmentInfo parent(info, false);
    for(std::list<Alias::SrcAlias>::iterator it=_list.begin();
      it!=_list.end(); it++) {
      if (it->src() == parent) {
        found = true;
        break;
      }
    }
    if (!found) {
      std::string parent_alias(alias_name, 0, alias_name.rfind('_'));
      _list.push_back(Alias::SrcAlias(parent, parent_alias.c_str()));
    }
  }
}

const char* AliasFactory::lookup(const Src& src) const
{
  std::list<Alias::SrcAlias>::const_iterator it;
  for (it = _list.begin(); it != _list.end(); it++) {
    if (it->src() == src) {
      return (it->aliasName());
    }
  }
  // no match found
  return ((char *)NULL);
}

AliasConfigType* AliasFactory::config(const PartitionConfigType* partn) const
{
  std::vector<Alias::SrcAlias> sources;
  for(std::list<Alias::SrcAlias>::const_iterator it=_list.begin();
      it!=_list.end(); it++) {
    bool lskip=(partn!=0);
    if (partn) {
      ndarray<const Partition::Source,1> s(partn->sources());
      for(unsigned i=0; i<s.size(); i++) {
        SegmentInfo parent(reinterpret_cast<const DetInfo&>(s[i]), false);
        if ((it->src()==s[i].src()) || (it->src()==parent)) {
          lskip=false;
          break;
        }
      }
    }
    if (!lskip)
      sources.push_back(*it);
  }

  unsigned sz = sizeof(AliasConfigType)+sources.size()*sizeof(Alias::SrcAlias);
  char* p = new char[sz];
  return new(p) AliasConfigType(sources.size(),sources.data());
}
