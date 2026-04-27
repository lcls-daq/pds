#ifndef Pds_PvDaq_PvMacros_hh
#define Pds_PvDaq_PvMacros_hh

#define CREATE_PV(pv, prefix, dest) \
  create_pv(pv, prefix, false, &dest, sizeof(dest), ConfigServer::NUMERIC)

#define CREATE_ENUM_PV(pv, prefix, dest, len) \
  create_pv(pv, prefix, true, dest, len, ConfigServer::STR)

#define CREATE_STR_PV(pv, prefix, dest, len)  \
  create_pv(pv, prefix, false, dest, len, ConfigServer::STR)

#define CREATE_LONGSTR_PV(pv, prefix, dest, len)  \
  create_pv(pv, prefix, false, dest, len, ConfigServer::LONGSTR)

#endif
