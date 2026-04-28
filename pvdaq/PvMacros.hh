#ifndef Pds_PvDaq_PvMacros_hh
#define Pds_PvDaq_PvMacros_hh

#include <sstream>
#include <string>

namespace Pds {
  namespace PvDaq {
    template <typename T>
    std::string enum_to_str(T value)
    {
      std::stringstream out;
      out << value;
      return out.str();
    }
  }
}

#define CREATE_PV(pv, prefix, dest) \
  create_pv(pv, prefix, false, &dest, sizeof(dest), ConfigServer::NUMERIC)

#define CREATE_ENUM_PV(pv, prefix, dest, len) \
  create_pv(pv, prefix, true, dest, len, ConfigServer::STR)

#define CREATE_STR_PV(pv, prefix, dest, len)  \
  create_pv(pv, prefix, false, dest, len, ConfigServer::STR)

#define CREATE_LONGSTR_PV(pv, prefix, dest, len)  \
  create_pv(pv, prefix, false, dest, len, ConfigServer::LONGSTR)

#endif
